#include <stdio.h>
extern "C" int xfib(int);
int xfib(int n){
	if (n<=2) {
		return 1;
	} else {
	    return xfib(n-1)+xfib(n-2);
    }
}
