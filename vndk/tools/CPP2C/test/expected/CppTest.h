#ifndef __CPPTESTWRAPPER_H_
#define __CPPTESTWRAPPER_H_
#ifdef __cplusplus
typedef bool _Bool;
extern "C"{
#endif
#include <stdbool.h>
#include <cstdint>
#include <stddef.h>
struct WCppClass;
typedef struct WCppClass WCppClass;

WCppClass* CppClass_create();
WCppClass* CppClass_create_1(int dummyInt);
WCppClass* CppClass_create_2(int first, char second, _Bool third);
void CppClass_destroy(WCppClass* self);

int CppClass_doFoo(WCppClass* self);
void CppClass_doFooThreeParameters(WCppClass* self, int first, char second, _Bool third);
int CppClass_doStaticFoo();
void CppClass_doStaticFooThreeParameters(int first, char second, _Bool third);
int CppClass_doOverloaded(WCppClass* self);
int CppClass_doOverloaded_1(WCppClass* self, int x);
void CppClass_noParameterName(WCppClass* self, int param0);
int CppClass_returnInt(WCppClass* self);
void CppClass_passInInt(WCppClass* self, int foo);
enum WCppEnum {
	kFirstElement,
	kSecondElement,
	kThirdElement,
};

WCppEnum CppClass_returnEnum();
void CppClass_passInEnum(WCppEnum foo);
char CppClass_returnChar(WCppClass* self);
void CppClass_passInChar(WCppClass* self, char foo);
struct WCppDummyClass;
typedef struct WCppDummyClass WCppDummyClass;

WCppDummyClass* CppClass_returnRef(WCppClass* self);
void CppClass_passPointer(WCppClass* self, WCppDummyClass* dummy);
void CppClass_passRef(WCppClass* self, const WCppDummyClass* dummy);
void CppClass_passByValue(WCppClass* self, WCppDummyClass* dummy);
WCppDummyClass* CppClass_returnByValue(WCppClass* self);
void CppClass_passFunctionPointer(WCppClass* self, void *(*foo) (int));
#ifdef __cplusplus
}
#endif
#endif /* __CPPTESTWRAPPER_H_ */

