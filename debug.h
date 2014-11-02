/*
 * Copyright(c) 2014 - Krister Hedfors
 */
#ifndef __DEBUG_H
#define __DEBUG_H

#ifndef DEBUG
#define debug(...)
#define _ENTER(...)
#define _ENTER0(...)
#define _LEAVE(...)
#else


#define DEBUG_FILE "/tmp/lfish.log"
FILE *debug_fp = NULL;


char const *__fnames[4096];
int __level = 0;


#define __DEBUG_INDENT do{                      \
  int __i = 0;                                  \
  for (__i = 0; __i < __level; __i++)           \
    fprintf(debug_fp, "    ");                  \
}while(0);


#define _debug(fmt, ...) do{                    \
  fprintf(debug_fp, "(%d)\t", __LINE__);        \
  __DEBUG_INDENT;			        \
  fprintf(debug_fp, fmt, ##__VA_ARGS__);        \
}while(0);


#define debug(fmt, ...) \
  _debug(fmt "\n", ##__VA_ARGS__)


#define _ENTER(fname, fmt, ...) do{             \
  __fnames[__level] = #fname;                   \
  fprintf(debug_fp, "(%d)\t", __LINE__);        \
  __DEBUG_INDENT                                \
  fprintf(debug_fp, "+ " #fname "(): " fmt "\n", ##__VA_ARGS__); \
  __level++;                                    \
}while(0);


#define _ENTER0(fname) _ENTER(fname, "")


#define _LEAVE do{                                \
  __level--;                                      \
  fprintf(debug_fp, "(%d)\t", __LINE__);          \
  __DEBUG_INDENT;                                 \
  fprintf(debug_fp, "- %s()\n", __fnames[__level]); \
}while(0);



#endif //DEBUG
#endif //__DEBUG_H
