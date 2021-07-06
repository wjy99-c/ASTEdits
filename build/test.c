void foo(int* a, int *b) {
  if (a[0] > 1)
  {
    b[0] = 2;
  }
  else
  {
    b[0] = 1;
  }
  for (int i = 0; i < 10; ++i) {
    b[0]++;
  }
}
int bar(float x, float y) {
  return 1;
} 