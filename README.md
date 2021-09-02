# ESP32 Compression

## Progress

### [zlib](https://github.com/madler/zlib)

- Compressed a txt file (see [demo.txt](assets/demo.txt)) to .gz archive with a C/R (compression ratio)  of ~2.25
- Inflated the .gz archive back to the original file on ESP32
- Compressed a .bin file (e.g Core Dump generated from ESP32 on panic) to .gz archive with a C/R of 5.00
- .gz archive can be extracted on a host machine (Linux, Windows)
- With a memory usage of 27K at peak, zlib gave a C/R of 2.38 for [demo.txt](assets/demo.txt)
- Config for above results -> window_bits: 12 | mem_level: 3 (See [zlib Manual - Advanced Functions](https://zlib.net/manual.html#Advanced))
- WIP: Binary files compression (e.g. OTA images, Core Dump images)

### [brotli](https://github.com/martinberlin/brotli)

- Compressed a txt file (see [demo.txt](assets/demo.txt)) to .br file with a C/R of ~2.17
- Decompressed the .br file on ESP32
- Pros: Fast Compression and decompression (about 8x faster than zlib)
- Cons: High binary size (about 600K of flash is used for brotli)
- Cons: High memory usage for compression and decompression (about 50-60K at peak!)
- Tried compressing the file in chunks (rather than all at once as given in example), but C/R fell with no significant in memory usage

### Other Attempts
- Compiled [miniz](https://github.com/richgel999/miniz) but could not get it working; always seem to run out of RAM
- WIP: Find more compression libraries with embedded systems support