#include "cwrapper.h"
#ifdef __cplusplus
extern "C"{
#endif
void Foo_doFoo(WFoo* self){
    reinterpret_cast<Foo*>(self)->doFoo(); 
}
#ifdef __cplusplus
}
#endif

