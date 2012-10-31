/* C code produced by gperf version 3.0.3 */
/* Command-line: gperf -G -L C -l -H type_hash -t -F ', 0' -N in_type_list -k '1,$' -W types type_hash.gperf  */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif


/* $Id: type_hash.gperf 1642 2008-08-10 02:22:04Z carenas $ */
/* Recognizes metric types. 
 * Build with: gperf -G -l -H type_hash -t -F ', 0' -N in_type_list -k 1,$ \
 * -W types ./type_hash.gperf > type_hash.c
 */
#include "../proto.h"
struct type_tag;

#define TOTAL_KEYWORDS 12
#define MIN_WORD_LENGTH 4
#define MAX_WORD_LENGTH 9
#define MIN_HASH_VALUE 4
#define MAX_HASH_VALUE 26
/* maximum key range = 23, duplicates = 0 */

inline static unsigned int type_hash(register const char *str,register unsigned int len)
{
  static unsigned char asso_values[] =
    {
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      15, 27, 10, 27,  5, 27,  0, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      10, 10,  9,  0, 27,  0, 27, 27, 27, 27,
      27, 27,  0, 27, 27,  0,  0,  0, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27, 27, 27, 27, 27,
      27, 27, 27, 27, 27, 27
    };
  return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

static unsigned char lengthtable[] =
  {
     0,  0,  0,  0,  4,  5,  6,  0,  0,  9,  5,  6,  0,  0,
     5,  5,  6,  0,  0,  0,  5,  6,  0,  0,  0,  0,  6
  };

static struct type_tag types[] =
  {
    {"", ERROR_TYPE}, {"", ERROR_TYPE}, {"", ERROR_TYPE}, {"", ERROR_TYPE},
    {"int8", INT},
    {"uint8", UINT},
    {"string", STRING},
    {"", ERROR_TYPE}, {"", ERROR_TYPE},
    {"timestamp", TIMESTAMP},
    {"int16", INT},
    {"uint16", UINT},
    {"", ERROR_TYPE}, {"", ERROR_TYPE},
    {"float", FLOAT},
    {"int64", INT},
    {"uint64", UINT},
    {"", ERROR_TYPE}, {"", ERROR_TYPE}, {"", ERROR_TYPE},
    {"int32", INT},
    {"uint32", UINT},
    {"", ERROR_TYPE}, {"", ERROR_TYPE}, {"", ERROR_TYPE}, {"", ERROR_TYPE},
    {"double", FLOAT}
  };

inline struct type_tag * in_type_list(register const char *str, register unsigned int len)
{
  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
      register int key = type_hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0) {
          if (len == lengthtable[key]) {
              register const char *s = types[key].name;

              if (*str == *s && !memcmp (str + 1, s + 1, len - 1)) {
                  return &types[key];
              }
          }
      }
  }

  return 0;
}
