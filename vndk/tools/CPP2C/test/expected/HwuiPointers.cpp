#include "test/output/HwuiPointersWrapper.h"
#include "test/input/HwuiPointers.h"
#include <type_traits>
#include <cstring>
#ifdef __cplusplus
extern "C"{
#endif
void RefCountedClass_destroy(WRefCountedClass* self){
    delete reinterpret_cast<dummynamespace::RefCountedClass*>(self); 
}
WRefCountedClass_sp* WRefCountedClass_sp_get(WRefCountedClass_sp* self){
	return reinterpret_cast<WRefCountedClass_sp*>(reinterpret_cast<android::sp<dummynamespace::RefCountedClass>*>(self)->get());
}

void WRefCountedClass_sp_delete(WRefCountedClass_sp* self){
	delete reinterpret_cast<android::sp<dummynamespace::RefCountedClass>*>(self);
}

WRefCountedClass_sp* HwuiTest_returnSp(){
    return reinterpret_cast<WRefCountedClass_sp*>(new android::sp<dummynamespace::RefCountedClass>(dummynamespace::HwuiTest::returnSp())); 
}
void HwuiTest_passSp(WRefCountedClass_sp* foo){
    dummynamespace::HwuiTest::passSp(*reinterpret_cast<android::sp<dummynamespace::RefCountedClass>*>(foo)); 
}
void HwuiTest_passSpRef(const WRefCountedClass_sp* foo){
    dummynamespace::HwuiTest::passSpRef(*reinterpret_cast<const android::sp<dummynamespace::RefCountedClass>*>(foo)); 
}
#ifdef __cplusplus
}
#endif

