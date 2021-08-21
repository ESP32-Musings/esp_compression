#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include "zlib_utils.h"

static const char *TAG = "spiffs_zlib";
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

    char *file_path = (char *)malloc(320 * sizeof(char));

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

void app_main(void)
{
    ESP_ERROR_CHECK(init_spiffs());
    ESP_LOGI(TAG, "Opening files for compression");

    // Compression
    FILE *source = fopen("/spiffs/demo.txt", "rb");
    // Appending .gz to the file name makes it extractable on host platforms
    FILE *comp = fopen("/spiffs/demo.txt.gz", "wb");

    if (comp == NULL || source == NULL) {
        ESP_LOGE(TAG, "Error opening file before compressing");
    }

    zerr(deflate_file(source, comp));
    ESP_LOGI(TAG, "Done compression");

    fclose(source);
    fclose(comp);

    // Inflation
    ESP_LOGI(TAG, "Start inflating");

    FILE *decomp = fopen("/spiffs/demo_u.txt", "wb");
    comp = fopen("/spiffs/demo.txt.gz", "rb");
    if (decomp == NULL || comp == NULL) {
        ESP_LOGE(TAG, "Error opening file before inflating");
    }

    zerr(inflate_file(comp, decomp));
    ESP_LOGI(TAG, "Done inflation");

    fclose(comp);
    fclose(decomp);

    get_spiffs_content(base_path);
}
