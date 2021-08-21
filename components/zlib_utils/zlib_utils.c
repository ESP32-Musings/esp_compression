#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include <sys/unistd.h>
#include <sys/stat.h>

#include "zlib.h"
#include "zlib_utils.h"

#define WINDOW_SIZE (CONFIG_WINDOW_SIZE | CONFIG_GZIP_ENCODING)
#define CHUNK_SIZE (CONFIG_WINDOW_SIZE) // Must be same as window size
#define MEM_LEVEL (CONFIG_MEM_LEVEL)
#define COMPRESSION_LEVEL (CONFIG_COMPRESSION_LEVEL)
#define COMPRESSION_STRATEGY (CONFIG_COMPRESSION_STRATEGY)

static const char *TAG = "zlib_utils";

void zerr(int ret)
{
    switch (ret) {
    case Z_ERRNO:
        ESP_LOGE(TAG, "File I/O Error");
        break;
    case Z_STREAM_ERROR:
        ESP_LOGE(TAG, "Invalid Compression level");
        break;
    case Z_DATA_ERROR:
        ESP_LOGE(TAG, "Data error");
        break;
    case Z_MEM_ERROR:
        ESP_LOGE(TAG, "Memory error");
        break;
    case Z_VERSION_ERROR:
        ESP_LOGE(TAG, "Version error");
        break;
    }
}

int deflate_file(FILE *source, FILE *dest)
{
    int ret, flush;
    unsigned have;
    z_stream strm;

    /*  Allocating on the stack only works for very small chunk sizes.
        unsigned char in[CHUNK_SIZE];
        unsigned char out[CHUNK_SIZE];
    */
    unsigned char *in = (unsigned char *)malloc(CHUNK_SIZE * sizeof(char));
    unsigned char *out = (unsigned char *)malloc(CHUNK_SIZE * sizeof(char));

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;

    ESP_LOGI(TAG, "Initiated Compression");

    ret = deflateInit2(&strm, COMPRESSION_LEVEL, Z_DEFLATED, WINDOW_SIZE, MEM_LEVEL, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
        return ret;
    }

    do {
        strm.avail_in = fread(in, 1, CHUNK_SIZE, source);
        if (ferror(source)) {
            zerr(deflateEnd(&strm));
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK_SIZE;
            strm.next_out = out;
            ret = deflate(&strm, flush);
            assert(ret != Z_STREAM_ERROR);
            zerr(ret);
            have = CHUNK_SIZE - strm.avail_out;

            if (fwrite(out, sizeof(char), have, dest) != have || ferror(dest)) {
                zerr(deflateEnd(&strm));
                return Z_ERRNO;
            }
        } while (strm.avail_out == 0);
        assert(strm.avail_in == 0);

    } while (flush != Z_FINISH);
    assert(ret == Z_STREAM_END);

    zerr(deflateEnd(&strm));
    free(in);
    free(out);
    return Z_OK;
}

int inflate_file(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;

    /*  Allocating on the stack only works for very small chunk sizes.
        unsigned char in[CHUNK_SIZE];
        unsigned char out[CHUNK_SIZE];
    */

    unsigned char *in = (unsigned char *)malloc(CHUNK_SIZE * sizeof(char));
    unsigned char *out = (unsigned char *)malloc(CHUNK_SIZE * sizeof(char));

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    ret = inflateInit2(&strm, WINDOW_SIZE); //Use window size in compressed stream
    if (ret != Z_OK) {
        return ret;
    }

    ESP_LOGI(TAG, "Initiated Decompression");

    do {
        strm.avail_in = fread(in, 1, CHUNK_SIZE, source);

        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0) {
            break;
        }
        strm.next_in = in;

        do {
            strm.avail_out = CHUNK_SIZE;
            strm.next_out = out;

            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }

            have = CHUNK_SIZE - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }

        } while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

    (void)inflateEnd(&strm);
    free(in);
    free(out);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
