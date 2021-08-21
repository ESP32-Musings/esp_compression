set(srcs
    "src/adler32.c"  
    "src/deflate.c"
    "src/gzread.c"
    "src/inffast.c"
    "src/trees.c"
    "src/zutil.c"
    "src/compress.c"
    "src/gzclose.c"
    "src/gzwrite.c"
    "src/inflate.c"
    "src/uncompr.c"
    "src/crc32.c"
    "src/gzlib.c"
    "src/infback.c"
    "src/inftrees.c")
    
idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include") 


