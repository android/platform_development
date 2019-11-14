#include "test/output/CppTestWrapper.h"
#include "test/input/CppTest.h"
#include <type_traits>
#include <cstring>
#ifdef __cplusplus
extern "C"{
#endif
WCppClass* CppClass_create(){
    return reinterpret_cast<WCppClass*>( new dummynamespace::CppClass()); 
}
WCppClass* CppClass_create_1(int dummyInt){
    return reinterpret_cast<WCppClass*>( new dummynamespace::CppClass(dummyInt)); 
}
WCppClass* CppClass_create_2(int first, char second, _Bool third){
    return reinterpret_cast<WCppClass*>( new dummynamespace::CppClass(first, second, third)); 
}
void CppClass_destroy(WCppClass* self){
    delete reinterpret_cast<dummynamespace::CppClass*>(self); 
}
void CppClass_passBuiltin(WCppClass* self, float foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passBuiltin(foo); 
}
void CppClass_passConstBuiltin(WCppClass* self, const float foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passConstBuiltin(foo); 
}
float CppClass_returnBuiltin(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->returnBuiltin(); 
}
const float CppClass_returnConstBuiltin(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->returnConstBuiltin(); 
}
void CppClass_doFooThreeParameters(WCppClass* self, int first, char second, _Bool third){
    reinterpret_cast<dummynamespace::CppClass*>(self)->doFooThreeParameters(first, second, third); 
}
int CppClass_returnStaticBuiltin(){
    return dummynamespace::CppClass::returnStaticBuiltin(); 
}
void CppClass_passStaticBuiltinThreeParameters(int first, char second, _Bool third){
    dummynamespace::CppClass::passStaticBuiltinThreeParameters(first, second, third); 
}
void CppClass_passArray(WCppClass* self, float foo[]){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passArray(foo); 
}
void CppClass_passConstArray(WCppClass* self, const float foo[]){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passConstArray(foo); 
}
void CppClass_passFixedSizeArray(WCppClass* self, float foo[9]){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passFixedSizeArray(foo); 
}
void CppClass_passConstFixedSizeArray(WCppClass* self, const float foo[9]){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passConstFixedSizeArray(foo); 
}
void CppClass_passBuiltinPointer(WCppClass* self, float* foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passBuiltinPointer(foo); 
}
void CppClass_passConstBuiltinPointer(WCppClass* self, const float* foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passConstBuiltinPointer(foo); 
}
float* CppClass_returnBuiltinPointer(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->returnBuiltinPointer(); 
}
const float* CppClass_returnConstBuiltinPointer(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->returnConstBuiltinPointer(); 
}
int CppClass_doOverloaded(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->doOverloaded(); 
}
int CppClass_doOverloaded_1(WCppClass* self, int x){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->doOverloaded(x); 
}
void CppClass_noParameterName(WCppClass* self, int param0){
    reinterpret_cast<dummynamespace::CppClass*>(self)->noParameterName(param0); 
}
WCppEnum CppClass_returnEnum(){
    return WCppEnum(static_cast<typename std::underlying_type<dummynamespace::CppEnum>::type>(dummynamespace::CppClass::returnEnum())); 
}
void CppClass_passInEnum(WCppEnum foo){
    dummynamespace::CppClass::passInEnum(static_cast<dummynamespace::CppEnum>(foo)); 
}
char CppClass_returnChar(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->returnChar(); 
}
void CppClass_passInChar(WCppClass* self, char foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passInChar(foo); 
}
void CppClass_passPointer(WCppClass* self, WCppDummyClass* dummy){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passPointer(reinterpret_cast<dummynamespace::CppDummyClass*>(dummy)); 
}
void CppClass_passConstPointer(WCppClass* self, const WCppDummyClass* foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passConstPointer(reinterpret_cast<const dummynamespace::CppDummyClass*>(foo)); 
}
WCppDummyClass* CppClass_returnPointer(WCppClass* self){
    return reinterpret_cast<WCppDummyClass*>(reinterpret_cast<dummynamespace::CppClass*>(self)->returnPointer()); 
}
const WCppDummyClass* CppClass_returnConstPointer(WCppClass* self){
    return reinterpret_cast<const WCppDummyClass*>(reinterpret_cast<dummynamespace::CppClass*>(self)->returnConstPointer()); 
}
void CppClass_passRef(WCppClass* self, WCppDummyClass* dummy){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passRef(*reinterpret_cast<dummynamespace::CppDummyClass*>(dummy)); 
}
void CppClass_passConstRef(WCppClass* self, const WCppDummyClass* dummy){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passConstRef(*reinterpret_cast<const dummynamespace::CppDummyClass*>(dummy)); 
}
WCppDummyClass* CppClass_returnRef(WCppClass* self){
    return reinterpret_cast<WCppDummyClass*>(&reinterpret_cast<dummynamespace::CppClass*>(self)->returnRef()); 
}
const WCppDummyClass* CppClass_returnConstRef(WCppClass* self){
    return reinterpret_cast<const WCppDummyClass*>(&reinterpret_cast<dummynamespace::CppClass*>(self)->returnConstRef()); 
}
void CppClass_passByValue(WCppClass* self, WCppDummyClass* dummy){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passByValue(*reinterpret_cast<dummynamespace::CppDummyClass*>(dummy)); 
}
WCppDummyClass* CppClass_returnByValue(WCppClass* self){
    return reinterpret_cast<WCppDummyClass*>(new dummynamespace::CppDummyClass(reinterpret_cast<dummynamespace::CppClass*>(self)->returnByValue())); 
}
void CppClass_passFunctionPointer(WCppClass* self, void *(*foo) (int)){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passFunctionPointer(foo); 
}
void CppClass_passBuiltinTypedef(WCppClass* self, float foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passBuiltinTypedef(foo); 
}
float CppClass_returnBuiltinTypedef(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->returnBuiltinTypedef(); 
}
void CppClass_passTypedef(WCppClass* self, WCppDummyClass* foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passTypedef(*reinterpret_cast<dummynamespace::CppDummyClass*>(foo)); 
}
WCppDummyClass* CppClass_returnTypedef(WCppClass* self){
    return reinterpret_cast<WCppDummyClass*>(new dummynamespace::CppDummyClass(reinterpret_cast<dummynamespace::CppClass*>(self)->returnTypedef())); 
}
void CppClass_passMemberTypedef(WCppClass* self, WCppDummyClass* foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passMemberTypedef(*reinterpret_cast<dummynamespace::CppDummyClass*>(foo)); 
}
WCppDummyClass* CppClass_returnMemberTypedef(WCppClass* self){
    return reinterpret_cast<WCppDummyClass*>(&reinterpret_cast<dummynamespace::CppClass*>(self)->returnMemberTypedef()); 
}
void CppClass_passStructTypedef(WCppClass* self, WCppStructTypedef* foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passStructTypedef(*reinterpret_cast<dummynamespace::CppStructTypedef*>(foo)); 
}
WCppStructTypedef* CppClass_returnStructTypedef(WCppClass* self){
    return reinterpret_cast<WCppStructTypedef*>(new dummynamespace::CppStructTypedef(reinterpret_cast<dummynamespace::CppClass*>(self)->returnStructTypedef())); 
}
#ifdef __cplusplus
}
#endif

