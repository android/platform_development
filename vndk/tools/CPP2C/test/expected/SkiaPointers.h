#ifndef __SKIAPOINTERSWRAPPER_H_
#define __SKIAPOINTERSWRAPPER_H_
#ifdef __cplusplus
typedef bool _Bool;
extern "C"{
#endif
#include <stdbool.h>
#include <cstdint>
#include <stddef.h>
struct WRefCountedClass;
typedef struct WRefCountedClass WRefCountedClass;

void RefCountedClass_destroy(WRefCountedClass* self);

struct WHwuiTest;
typedef struct WHwuiTest WHwuiTest;

struct WRefCountedClass_sk_sp;
typedef struct WRefCountedClass_sk_sp WRefCountedClass_sk_sp;
WRefCountedClass_sk_sp* WRefCountedClass_sk_sp_get(WRefCountedClass_sk_sp* self);
void WRefCountedClass_sk_sp_delete(WRefCountedClass_sk_sp* self);

WRefCountedClass_sk_sp* HwuiTest_returnSksp();
void HwuiTest_passSksp(WRefCountedClass_sk_sp* foo);
void HwuiTest_passStrongPointerRef(const WRefCountedClass_sk_sp* foo);
#ifdef __cplusplus
}
#endif
#endif /* __SKIAPOINTERSWRAPPER_H_ */

