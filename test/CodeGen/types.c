// RUN: clang -emit-llvm <%s

struct FileName {
    struct FileName *next;
} *fnhead;


struct ieeeExternal {
    struct ieeeExternal *next;
} *exthead;


void test1()
{
    struct ieeeExternal *exttmp = exthead;
}

struct MpegEncContext;
typedef struct MpegEncContext {int pb;} MpegEncContext;
static void test2(void) {MpegEncContext s; s.pb;}

