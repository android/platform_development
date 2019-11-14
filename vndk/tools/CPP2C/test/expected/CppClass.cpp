#include "cwrapper.h"
#include <type_traits> // for std::underlying_type usage
#ifdef __cplusplus
extern "C"{
#endif
WCppClass* CppClass_create(){
    return reinterpret_cast<WCppClass*>( new dummynamespace::CppClass()); 
}
WCppClass* CppClass_create_1(int dummyInt){
    return reinterpret_cast<WCppClass*>( new dummynamespace::CppClass(dummyInt)); 
}
void CppClass_destroy(WCppClass* self){
    delete reinterpret_cast<dummynamespace::CppClass*>(self); 
}
int CppClass_doFoo(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->doFoo(); 
}
int CppClass_doStaticFoo(){
    return dummynamespace::CppClass::doStaticFoo(); 
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
int CppClass_returnInt(WCppClass* self){
    return reinterpret_cast<dummynamespace::CppClass*>(self)->returnInt(); 
}
void CppClass_passInInt(WCppClass* self, int foo){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passInInt(foo); 
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
WCppDummyClass* CppClass_returnRef(WCppClass* self){
    return reinterpret_cast<WCppDummyClass*>(&reinterpret_cast<dummynamespace::CppClass*>(self)->returnRef()); 
}
void CppClass_passPointer(WCppClass* self, WCppDummyClass* dummy){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passPointer(reinterpret_cast<dummynamespace::CppDummyClass*>(dummy)); 
}
void CppClass_passRef(WCppClass* self, const WCppDummyClass* dummy){
    reinterpret_cast<dummynamespace::CppClass*>(self)->passRef(*reinterpret_cast<const dummynamespace::CppDummyClass*>(dummy)); 
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
#ifdef __cplusplus
}
#endif

