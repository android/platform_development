#include <stdio.h>

extern void map(int arr[], int n);

int main() {
  int arr[5] = {1, 2, 3, 4, 5};
  map(arr, 5);
  for (int i = 0; i < 5; i ++)
    printf("%d\n", arr[i]);
}
