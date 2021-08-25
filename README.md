# ESP32 Compression

## Progress

### [zlib](https://github.com/madler/zlib)

- Compressed a txt file (see [demo.txt](demo.txt)) to .gz archive with a C/R  of ~2.25
- Inflated the .gz archive back to the original file on ESP32
- Compressed a .bin file (e.g Core Dump generated from ESP32 on panic) to .gz archive with a C/R of 5.00
- .gz archive can be extracted on a host machine (Linux, Windows)
- WIP: Optimize zlib parameters for ESP32 (maximum compression with minimum memory usage)
- To-do: Compress streams of data rather than a file

### [brotli](https://github.com/martinberlin/brotli)

- Compressed a txt file (see [demo.txt](demo.txt)) to .br file with a compression ratio of ~2.17
- WIP: Fix failure to decompress the .br file on ESP32
- To-do: Compress streams of data rather than a file

### Others

- Compiled [miniz](https://github.com/richgel999/miniz) but could not get it working; always seem to run out of RAM
- To-do: Benchmarking compression with different types of files