#include "test/output/SkiaPointersWrapper.h"
#include "test/input/SkiaPointers.h"
#include <type_traits>
#include <cstring>
#ifdef __cplusplus
extern "C"{
#endif
void RefCountedClass_destroy(WRefCountedClass* self){
    delete reinterpret_cast<dummynamespace::RefCountedClass*>(self); 
}
WRefCountedClass_sk_sp* WRefCountedClass_sk_sp_get(WRefCountedClass_sk_sp* self){
	return reinterpret_cast<WRefCountedClass_sk_sp*>(reinterpret_cast<::sk_sp<dummynamespace::RefCountedClass>*>(self)->get());
}

void WRefCountedClass_sk_sp_delete(WRefCountedClass_sk_sp* self){
	delete reinterpret_cast<::sk_sp<dummynamespace::RefCountedClass>*>(self);
}

WRefCountedClass_sk_sp* HwuiTest_returnSksp(){
    return reinterpret_cast<WRefCountedClass_sk_sp*>(new ::sk_sp<dummynamespace::RefCountedClass>(dummynamespace::HwuiTest::returnSksp())); 
}
void HwuiTest_passSksp(WRefCountedClass_sk_sp* foo){
    dummynamespace::HwuiTest::passSksp(*reinterpret_cast<::sk_sp<dummynamespace::RefCountedClass>*>(foo)); 
}
void HwuiTest_passStrongPointerRef(const WRefCountedClass_sk_sp* foo){
    dummynamespace::HwuiTest::passStrongPointerRef(*reinterpret_cast<const ::sk_sp<dummynamespace::RefCountedClass>*>(foo)); 
}
#ifdef __cplusplus
}
#endif

