#ifndef __CPPSTDTESTWRAPPER_H_
#define __CPPSTDTESTWRAPPER_H_
#ifdef __cplusplus
typedef bool _Bool;
extern "C"{
#endif
#include <stdbool.h>
#include <cstdint>
#include <stddef.h>

struct WCppClass;
typedef struct WCppClass WCppClass;

WCppClass* CppClass_returnUniquePointer();
void CppClass_passUniquePointer(WCppClass* foo);
struct WCppClass_shared_ptr;
typedef struct WCppClass_shared_ptr WCppClass_shared_ptr;
WCppClass_shared_ptr* WCppClass_shared_ptr_get(WCppClass_shared_ptr* self);
void WCppClass_shared_ptr_delete(WCppClass_shared_ptr* self);

WCppClass_shared_ptr* CppClass_returnSharedPtr();
void CppClass_passSharedPtr(WCppClass_shared_ptr* foo);
char* CppClass_returnString();
void CppClass_passString(char* foo);
#ifdef __cplusplus
}
#endif
#endif /* __CPPSTDTESTWRAPPER_H_ */

