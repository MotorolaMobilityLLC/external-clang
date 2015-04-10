#ifndef A_SYSTEM_H
#define A_SYSTEM_H

// FIXME: We have to use this to mark the header as a system header in
// a module because header search didn't actually occur and so we can't have
// found the header via system header search, even though when we map to this
// header and load the module we will have mapped to the header by finding it
// via system header search.
#pragma GCC system_header

#define FOO1_SYSTEM(x) x + x
#define BAR1_SYSTEM(x) x + x
#define BAZ1_SYSTEM(x) x + x

#endif
