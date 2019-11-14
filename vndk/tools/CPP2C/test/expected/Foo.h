#ifndef _FOO_CWRAPPER_H_
#define _FOO_CWRAPPER_H_
#include "/usr/local/google/home/catalintudor/android/internal/aosp-master/development/vndk/tools/CPP2C/test/input/Foo.h"
#ifdef __cplusplus
typedef bool _Bool;
extern "C"{
#endif
#include <stdbool.h>
struct WFoo; 
typedef struct WFoo WFoo;

void Foo_doFoo(WFoo* self);
#ifdef __cplusplus
}
#endif
#endif /* _FOO_CWRAPPER_H_ */

