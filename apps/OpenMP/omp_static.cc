void map(int arr[], int n) {
#pragma omp parallel
  for (int i = 0; i < n; i ++)
    arr[i] *= 2;
}
