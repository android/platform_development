#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

typedef int (*f)(int);
int (*xfib)(int);

int fib(int n){
	if (n<=10) {
        return xfib(n);
	} else {
        if (n ==25)
            printf("fib! %d\n",n);
	    return fib(n-1)+fib(n-2);
    }
}

int main(){
    void *handle = dlopen ("libprof-test.so", RTLD_LAZY);;
    xfib = (f)dlsym(handle, "xfib");
    printf("start test profiling !\n");
    printf("fib(40) =%d\n", fib(40));
    printf("dlclose start\n");
    //dlclose(handle);
    printf("dlclose done\n");
    return 0;
}
