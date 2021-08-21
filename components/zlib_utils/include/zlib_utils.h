#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"

void zerr(int ret);

int deflate_file(FILE *source, FILE *dest);

int inflate_file(FILE *source, FILE *dest);