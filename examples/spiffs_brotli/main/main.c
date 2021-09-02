#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <sys/unistd.h>
#include <sys/stat.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

#include "esp_spiffs.h"
#include "esp_timer.h"

#include "brotli/decode.h"
#include "brotli/encode.h"

// Range: 1 - 11 (max. compression); NOTE: ESP32 crashes for > 1
#define COMPRESSION_QUALITY (1)
#define BROTLI_BUFFER (4096)

#define DEMO_TXT "/spiffs/demo.txt"
#define DEMO_TXT_BR "/spiffs/demo.txt.br"
#define DEMO_U_TXT "/spiffs/demo_u.txt"

static const char *TAG = "spiffs_brotli";
 
esp_err_t init_spiffs(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGI(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    //Getting SPIFFS info
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ESP_OK;
}

void get_spiffs_content(void)
{
    const char *base_path = "/spiffs/";
    struct dirent *de;
    struct stat st;
    DIR *dr = opendir(base_path);

    char *file_path = malloc(512 * sizeof(char));

    if (dr == NULL) {
        ESP_LOGE(TAG, "Could not open current directory" );
        return;
    }

    while ((de = readdir(dr)) != NULL) {
        sprintf(file_path, "%s%s", base_path, de->d_name);
        stat(file_path, &st);
        ESP_LOGI(TAG, "File: %s, Size: %ld bytes", de->d_name, st.st_size);
    }

    closedir(dr);
    free(file_path);
    return;
}

int get_file_size(char *file_path)
{
    struct stat st; 
    if (stat(file_path, &st) == 0)
        return st.st_size;
    return -1; 
}

void decompress_file(void* arg)
{
    FILE *file = fopen(DEMO_TXT_BR, "rb");
    int fileSize = get_file_size(DEMO_TXT_BR);

    uint8_t *buffer = calloc(BROTLI_BUFFER, sizeof(uint8_t));
    char *inBuffer = calloc(fileSize, sizeof(char));

    fread(inBuffer, 1, fileSize, file);
    fclose(file);

    size_t decodedSize = BROTLI_BUFFER;
    ESP_LOGI(TAG, "Starting Decompression...");
    
    // TODO: Decompress in chunks rather than all at once
    int64_t start = esp_timer_get_time();     
    int brotliStatus = BrotliDecoderDecompress(fileSize, (const uint8_t *)inBuffer, &decodedSize, buffer);
    int64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Decompression time: %lld", end - start);  

    if (brotliStatus == BROTLI_DECODER_RESULT_ERROR){
        ESP_LOGE(TAG, "Decompression Failed!");
        goto CLEANUP;
    }

    FILE *dest = fopen(DEMO_U_TXT, "w");
    fwrite(buffer, 1, decodedSize, dest);
    fclose(dest);

CLEANUP: 
    free(inBuffer);
    free(buffer);

    ESP_LOGI(TAG, "Free heap (Minimum): %d", esp_get_minimum_free_heap_size());
    vTaskDelete(NULL);
}

void compress_file(void *arg)
{
    uint8_t *buffer = calloc(BROTLI_BUFFER, sizeof(uint8_t));

    FILE *source = fopen(DEMO_TXT, "r");
    int fileSize = get_file_size(DEMO_TXT);

    char *inBuffer = calloc(fileSize, sizeof(char));
    fread(inBuffer, 1, fileSize, source);
    fclose(source);

    size_t encodedSize = BROTLI_BUFFER;
    ESP_LOGI(TAG, "Starting Compression...");  

    // TODO: Compress in chunks rather than compress the file all at once; will result a drop in compression ratio
    int64_t start = esp_timer_get_time();     
    bool brotliStatus = BrotliEncoderCompress(COMPRESSION_QUALITY, BROTLI_DEFAULT_WINDOW, BROTLI_DEFAULT_MODE, 
                                                fileSize, (const uint8_t *)inBuffer, &encodedSize, buffer);
    int64_t end = esp_timer_get_time();
    ESP_LOGI(TAG, "Compression time: %lld", end - start);  

    if(!brotliStatus){
        ESP_LOGE(TAG, "Compression Failed!");
        goto CLEANUP;
    }

    FILE *dest = fopen(DEMO_TXT_BR, "wb");
    fwrite(buffer, 1, encodedSize, dest);
    fclose(dest);

    ESP_LOGI(TAG, "Compression Ratio: %0.2f", (float)fileSize / encodedSize);

CLEANUP: 
    free(inBuffer);
    free(buffer);

    ESP_LOGI(TAG, "Free heap (Minimum): %d", esp_get_minimum_free_heap_size());
    vTaskDelete(NULL);
}


void app_main(void)
{
    ESP_LOGI(TAG, "Opening files for compression");
    ESP_ERROR_CHECK(init_spiffs());

    ESP_LOGI(TAG, "Free heap (Start): %d", esp_get_free_heap_size());

    xTaskCreate(compress_file, "compress", 8192, NULL, 5, NULL);
    // xTaskCreate(decompress_file, "decompress", 8192, NULL, 5, NULL);

    get_spiffs_content();
}
