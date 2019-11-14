#include "test/output/CppStdTestWrapper.h"
#include "test/input/CppStdTest.h"
#include <type_traits>
#include <cstring>
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
char* stringToC(const std::string& s) {
	char* copy;
	copy = (char*)(malloc(s.size() * sizeof(char) + 1));
	strncpy(copy, s.c_str(), s.size());
	copy[s.size() + 1] = '\0';
	return copy;
}
char* CppClass_returnString(){
    return stringToC(dummynamespace::CppClass::returnString()); 
}
void CppClass_passString(char* foo){
    dummynamespace::CppClass::passString(std::basic_string<char>(foo)); 
}
#ifdef __cplusplus
}
#endif

