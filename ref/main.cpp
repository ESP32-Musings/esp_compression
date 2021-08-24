extern "C" {
#include <brotli/decode.h>
#include <brotli/encode.h>
}
#include "Arduino.h"
#include "FS.h"
#include "SPIFFS.h"

#define DEFAULT_LGWIN 22
#define BROTLI_BUFFER 30000;
size_t fileSize;

File file;

uint8_t * decompressBuffer(uint8_t *inBuffer, size_t inputSize) {
  int bufferSize = BROTLI_BUFFER;
  uint8_t * buffer = new uint8_t[bufferSize];
  size_t output_length = bufferSize;
  Serial.println("decompressBuffer() - - - - - - - - - - - - - - - - - - - - - - - - - -");

  bool brotliStatus;
  brotliStatus = BrotliDecoderDecompress(
    inputSize,
    (const uint8_t *)inBuffer,
    &output_length,
    buffer);

  Serial.println();
  Serial.printf("status: %d\n", brotliStatus);
  Serial.println("Decompress output: - - - - - - - - - - - - - - - - - - - - - - - - - -");

  Serial.printf("%.*s\n", output_length, buffer);
  
  return buffer;
}

bool decompressFile(String fileName, bool outputBytes=false) {
  if (!SPIFFS.exists(fileName)) {
    Serial.println(fileName+ " not found. Did you execute: pio run -t uploadfs");
    return false;
  }
  bool brotliStatus;
  file = SPIFFS.open(fileName, "r"); 
  fileSize = file.size();
  char *inBuffer = new char[fileSize];
  file.readBytes(inBuffer, fileSize);
  file.close();
  
  //Serial.printf("%d bytes read into inBuffer from: ", fileSize);
  //Serial.print(fileName+ "\n");

  int decompressTime = micros();
  // Uncomment for easy test withouth SPIFFS
  //uint8_t inBuffer[] = {27, 175, 4,248, 141, 148, 110, 222, 68, 85, 134, 214, 32, 33, 108, 111, 106, 22, 199, 106, 129, 12, 168, 102, 47, 4};
  int bufferSize = BROTLI_BUFFER;
  uint8_t *buffer = new uint8_t[bufferSize];
  size_t output_length = bufferSize;

  brotliStatus = BrotliDecoderDecompress(
    fileSize,
    (const uint8_t *)inBuffer,
    &output_length,
    buffer);
  
  delete(inBuffer);
  delete(buffer);

  int timespent = micros()-decompressTime;
  int pixelslen = (output_length-5)/4;

  //Serial.printf("Decompression took %d micros. File read took: %d micros\n", timespent, decompressTime-readFsTime);
  //Serial.printf("%d bytes after decompression. Estimated pixels RGBw: %d\n", output_length, (output_length-5)/4);
  Serial.print(fileName+";"+String(pixelslen)+";"+String(fileSize)+";");
  Serial.print(String(output_length)+";");
  Serial.print(String(timespent)+";");
  Serial.print(String((pixelslen/300)*9)+";");
  Serial.println();

  if (outputBytes) {
    for ( int i = 0; i < output_length; i++ ) {
      uint8_t conv = (int) buffer[i];
      Serial.print(conv);Serial.print(",");
    }
    Serial.printf("%.*s\n", output_length, buffer);
  }
  return brotliStatus;
}


uint8_t * compressFile(String fileName, bool outputBytes=false) {
  
  int lgwin = DEFAULT_LGWIN;
  int bufferSize = BROTLI_BUFFER;
  uint8_t *buffer = new uint8_t[bufferSize];

  if (!SPIFFS.exists(fileName)) {
    Serial.println(fileName+ " not found. Did you execute: pio run -t uploadfs");
    return buffer;
  }
  Serial.println("Reading file '"+fileName+"' from FS");

  file = SPIFFS.open(fileName, "r"); 
  fileSize = file.size();
  char *inBuffer = new char[fileSize];
  file.readBytes(inBuffer, fileSize);
  file.close();
  Serial.println("- - - - - - - - - - - - - - - - - - - - - - - - - -");
  Serial.printf("%d bytes read into inBuffer from: ", fileSize);
  Serial.print(fileName+ "\n");

  // 1 less to 11 max. compression. With more than 1 it hangs on ESP32
  int quality = 1; 
  size_t encodedSize = bufferSize;

  Serial.println("Calling BrotliEncoderCompress");
  Serial.printf("Compression params:\n%d quality\n%d lgwin", quality, lgwin);

  // ***ERROR*** lgwinabort() increasing quality >1
  int decompressTime = micros();
  bool brotliStatus;

   brotliStatus = BrotliEncoderCompress(
    quality,  
    lgwin, 
    BrotliEncoderMode::BROTLI_MODE_GENERIC,
    fileSize, 
    (const uint8_t *)inBuffer,     
    &encodedSize,
    buffer);  

  int timespent = micros()-decompressTime;

  delete(inBuffer);
  delete(buffer);
  Serial.println();
  Serial.printf("status: %d\n", brotliStatus);
  Serial.printf("%d microseconds spend compressing\n", timespent);
  Serial.printf("%d bytes after compression\n", encodedSize);
 
  if (outputBytes) {
    Serial.println("Compressed bytes preview:");
    for ( int i = 0; i < encodedSize; i++ ) {
      uint8_t conv = (int) buffer[i];
      Serial.print(conv);Serial.print(",");
    }
    Serial.printf("%.*s\n", encodedSize, buffer);
  }  

  return buffer;
}

void setup() {
  Serial.begin(115200);
  
  if (!SPIFFS.begin()) {
   Serial.println("Could not mount file system");
 }

Serial.println("- - - - - - - - - - - - - - - - - - - - - - - - - -");
Serial.println("Filename;Pixels;Compressed size(byte);Decompressed size;Decomp. micros;Neopixel process millis;");

// Decompress example with other files (Outputs a small CSV)
/*   decompressFile("/500-1.bin.br");
  decompressFile("/500-r.bin.br");
  decompressFile("/1000-1.bin.br");
  decompressFile("/1000-r.bin.br");
  decompressFile("/2000-1.bin.br");
  decompressFile("/4000-1.bin.br");
  decompressFile("/6000-1.bin.br"); */
  
  // Read a file from FS
  int bufferSize = BROTLI_BUFFER;
  uint8_t *inBuffer = new uint8_t[bufferSize];
  inBuffer = compressFile("/sensors.json");
  
  // Decompress the compressed bytes output
  decompressBuffer(inBuffer, bufferSize);
 
  // Note: That bufferSize should be ^ ideally the size of the inBuffer (349 in case of that file)


  compressFile("/500-1.bin");
  compressFile("/500-r.bin");
  compressFile("/1000-1.bin");
  compressFile("/1000-r.bin");
}
 
void loop() {
  delay(100);
}