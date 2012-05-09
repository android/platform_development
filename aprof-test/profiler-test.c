#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

int xfib(int n);
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
    printf("start test profiling !\n");
    printf("fib(40) =%d\n", fib(40));
    return 0;
}
