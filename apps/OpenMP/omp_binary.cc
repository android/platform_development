#include <stdio.h>

void map(int arr[], int n) {
#pragma omp parallel
  for (int i = 0; i < n; i ++)
    arr[i] *= 2;
}

int main() {
  int arr[5] = {1, 2, 3, 4, 5};
  map(arr, 5);
  for (int i = 0; i < 5; i ++)
    printf("%d\n", arr[i]);
}
