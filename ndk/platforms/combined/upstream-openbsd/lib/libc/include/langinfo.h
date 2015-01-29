/* Hack to build "vfprintf.c". */
#define RADIXCHAR 1
#define nl_langinfo(i) ((i == RADIXCHAR) ? (char*) "." : NULL)
