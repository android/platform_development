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

void CppClass_passBuiltin(WCppClass* self, float foo);
void CppClass_passConstBuiltin(WCppClass* self, const float foo);
float CppClass_returnBuiltin(WCppClass* self);
const float CppClass_returnConstBuiltin(WCppClass* self);
void CppClass_doFooThreeParameters(WCppClass* self, int first, char second, _Bool third);
int CppClass_returnStaticBuiltin();
void CppClass_passStaticBuiltinThreeParameters(int first, char second, _Bool third);
void CppClass_passArray(WCppClass* self, float foo[]);
void CppClass_passConstArray(WCppClass* self, const float foo[]);
void CppClass_passFixedSizeArray(WCppClass* self, float foo[9]);
void CppClass_passConstFixedSizeArray(WCppClass* self, const float foo[9]);
void CppClass_passBuiltinPointer(WCppClass* self, float* foo);
void CppClass_passConstBuiltinPointer(WCppClass* self, const float* foo);
float* CppClass_returnBuiltinPointer(WCppClass* self);
const float* CppClass_returnConstBuiltinPointer(WCppClass* self);
int CppClass_doOverloaded(WCppClass* self);
int CppClass_doOverloaded_1(WCppClass* self, int x);
void CppClass_noParameterName(WCppClass* self, int param0);
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

void CppClass_passPointer(WCppClass* self, WCppDummyClass* dummy);
void CppClass_passConstPointer(WCppClass* self, const WCppDummyClass* foo);
WCppDummyClass* CppClass_returnPointer(WCppClass* self);
const WCppDummyClass* CppClass_returnConstPointer(WCppClass* self);
void CppClass_passRef(WCppClass* self, WCppDummyClass* dummy);
void CppClass_passConstRef(WCppClass* self, const WCppDummyClass* dummy);
WCppDummyClass* CppClass_returnRef(WCppClass* self);
const WCppDummyClass* CppClass_returnConstRef(WCppClass* self);
void CppClass_passByValue(WCppClass* self, WCppDummyClass* dummy);
WCppDummyClass* CppClass_returnByValue(WCppClass* self);
void CppClass_passFunctionPointer(WCppClass* self, void *(*foo) (int));
void CppClass_passBuiltinTypedef(WCppClass* self, float foo);
float CppClass_returnBuiltinTypedef(WCppClass* self);
void CppClass_passTypedef(WCppClass* self, WCppDummyClass* foo);
WCppDummyClass* CppClass_returnTypedef(WCppClass* self);
void CppClass_passMemberTypedef(WCppClass* self, WCppDummyClass* foo);
WCppDummyClass* CppClass_returnMemberTypedef(WCppClass* self);
struct WCppStructTypedef;
typedef struct WCppStructTypedef WCppStructTypedef;

void CppClass_passStructTypedef(WCppClass* self, WCppStructTypedef* foo);
WCppStructTypedef* CppClass_returnStructTypedef(WCppClass* self);
#ifdef __cplusplus
}
#endif
#endif /* __CPPTESTWRAPPER_H_ */

