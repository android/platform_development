#ifndef __HWUIPOINTERSWRAPPER_H_
#define __HWUIPOINTERSWRAPPER_H_
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

struct WRefCountedClass_sp;
typedef struct WRefCountedClass_sp WRefCountedClass_sp;
WRefCountedClass_sp* WRefCountedClass_sp_get(WRefCountedClass_sp* self);
void WRefCountedClass_sp_delete(WRefCountedClass_sp* self);

WRefCountedClass_sp* HwuiTest_returnSp();
void HwuiTest_passSp(WRefCountedClass_sp* foo);
void HwuiTest_passSpRef(const WRefCountedClass_sp* foo);
#ifdef __cplusplus
}
#endif
#endif /* __HWUIPOINTERSWRAPPER_H_ */

