#include "test/output/CppStdTestWrapper.h"
#include "test/input/CppStdTest.h"
#include <type_traits>
#ifdef __cplusplus
extern "C"{
#endif
WCppClass* CppClass_returnUniquePointer(){
    return reinterpret_cast<WCppClass*>(dummynamespace::CppClass::returnUniquePointer().release()); 
}
void CppClass_passUniquePointer(WCppClass* foo){
    dummynamespace::CppClass::passUniquePointer(std::move(std::unique_ptr<dummynamespace::CppClass>(reinterpret_cast<dummynamespace::CppClass*>(foo)))); 
}
WCppClass_shared_ptr* WCppClass_shared_ptr_get(WCppClass_shared_ptr* self){
	return reinterpret_cast<WCppClass_shared_ptr*>(reinterpret_cast<std::shared_ptr<dummynamespace::CppClass>*>(self)->get());
}

void WCppClass_shared_ptr_delete(WCppClass_shared_ptr* self){
	delete reinterpret_cast<std::shared_ptr<dummynamespace::CppClass>*>(self);
}

WCppClass_shared_ptr* CppClass_returnSharedPtr(){
    return reinterpret_cast<WCppClass_shared_ptr*>(new std::shared_ptr<dummynamespace::CppClass>(dummynamespace::CppClass::returnSharedPtr())); 
}
void CppClass_passSharedPtr(WCppClass_shared_ptr* foo){
    dummynamespace::CppClass::passSharedPtr(*reinterpret_cast<std::shared_ptr<dummynamespace::CppClass>*>(foo)); 
}
#ifdef __cplusplus
}
#endif

