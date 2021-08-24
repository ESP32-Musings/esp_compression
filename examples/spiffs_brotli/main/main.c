#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "decode.h"
#include "encode.h"

#define DEFAULT_LGWIN (22)
#define BROTLI_BUFFER (32768)

static const char *TAG = "spiffs_brotli";
const char *base_path = "/spiffs/";

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

void get_spiffs_content(const char *base_path)
{
    struct dirent *de;
    struct stat st;
    DIR *dr = opendir(base_path);

    char *file_path = malloc(256 * sizeof(char));

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

void compress_file(char *fileName)
{
    int lgwin = DEFAULT_LGWIN;
    int bufferSize = BROTLI_BUFFER;
    uint8_t *buffer = calloc(bufferSize, sizeof(uint8_t));

    FILE *file = fopen(fileName, "rb");
    int fileSize = get_file_size(fileName);

    char *inBuffer = calloc(fileSize, sizeof(char));
    fread(inBuffer, fileSize, 1, file);
    fclose(file);

    // 1 less to 11 max. compression. With more than 1 it hangs on ESP32
    int quality = 1; 
    size_t encodedSize = bufferSize;

    ESP_LOGI(TAG, "Starting Compression...");
    bool brotliStatus = BrotliEncoderCompress(quality, lgwin, BROTLI_MODE_GENERIC, 
                                                fileSize, (const uint8_t *)inBuffer, &encodedSize, buffer);  

    free(inBuffer);
    free(buffer);

    ESP_LOGI(TAG, "Compression-> Before: %d | After: %d", fileSize, encodedSize);
    ESP_LOGI(TAG, "Compression Ratio: %0.2f", (float)fileSize / encodedSize);
}

void app_main(void)
{
    ESP_ERROR_CHECK(init_spiffs());
    ESP_LOGI(TAG, "Opening files for compression");

    compress_file("/spiffs/demo.txt");

    get_spiffs_content(base_path);
}