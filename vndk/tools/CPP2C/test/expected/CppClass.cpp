#include "cwrapper.h"
#ifdef __cplusplus
extern "C"{
#endif
WCppDummyClass* CppDummyClass_create(){
    return reinterpret_cast<WCppDummyClass*>( new CppDummyClass()); 
}
void CppDummyClass_destroy(WCppDummyClass* self){
    delete reinterpret_cast<CppDummyClass*>(self); 
}
WCppClass* CppClass_create(){
    return reinterpret_cast<WCppClass*>( new CppClass()); 
}
WCppClass* CppClass_create_1(int dummyInt){
    return reinterpret_cast<WCppClass*>( new CppClass(dummyInt)); 
}
void CppClass_destroy(WCppClass* self){
    delete reinterpret_cast<CppClass*>(self); 
}
int CppClass_doFoo(WCppClass* self){
    return reinterpret_cast<CppClass*>(self)->doFoo(); 
}
int CppClass_doStaticFoo(){
    return CppClass::doStaticFoo(); 
}
int CppClass_doOverloaded(WCppClass* self){
    return reinterpret_cast<CppClass*>(self)->doOverloaded(); 
}
int CppClass_doOverloaded_1(WCppClass* self, int x){
    return reinterpret_cast<CppClass*>(self)->doOverloaded(x); 
}
void CppClass_noParameterName(WCppClass* self, int param0){
    reinterpret_cast<CppClass*>(self)->noParameterName(param0); 
}
int CppClass_returnInt(WCppClass* self){
    return reinterpret_cast<CppClass*>(self)->returnInt(); 
}
void CppClass_passInInt(WCppClass* self, int foo){
    reinterpret_cast<CppClass*>(self)->passInInt(foo); 
}
char CppClass_returnChar(WCppClass* self){
    return reinterpret_cast<CppClass*>(self)->returnChar(); 
}
void CppClass_passInChar(WCppClass* self, char foo){
    reinterpret_cast<CppClass*>(self)->passInChar(foo); 
}
WCppDummyClass* CppClass_returnRef(WCppClass* self){
    return reinterpret_cast<WCppDummyClass*>(&reinterpret_cast<CppClass*>(self)->returnRef()); 
}
void CppClass_passPointer(WCppClass* self, WCppDummyClass* dummy){
    reinterpret_cast<CppClass*>(self)->passPointer(reinterpret_cast<CppDummyClass*>(dummy)); 
}
void CppClass_passRef(WCppClass* self, const WCppDummyClass* dummy){
    reinterpret_cast<CppClass*>(self)->passRef(*reinterpret_cast<const CppDummyClass*>(dummy)); 
}
void CppClass_passByValue(WCppClass* self, WCppDummyClass* dummy){
    reinterpret_cast<CppClass*>(self)->passByValue(*reinterpret_cast<CppDummyClass*>(dummy)); 
}
WCppDummyClass* CppClass_returnByValue(WCppClass* self){
    return reinterpret_cast<WCppDummyClass*>(new CppDummyClass(reinterpret_cast<CppClass*>(self)->returnByValue())); 
}
void CppClass_passFunctionPointer(WCppClass* self, void *(*foo) (int)){
    reinterpret_cast<CppClass*>(self)->passFunctionPointer(foo); 
}
#ifdef __cplusplus
}
#endif

