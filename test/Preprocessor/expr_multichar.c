// RUN: clang-cc < %s -E -verify -triple i686-pc-linux-gnu

#if (('1234' >> 24) != '1')
#error Bad multichar constant calculation!
#endif
