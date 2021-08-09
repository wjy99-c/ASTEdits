
int bar(float x, float y) {
  return 1;
} 

void foo(int a[], int b[], int N);

int main(int argc, char const *argv[])
{
  int a[10];
  int b[10];
  foo(a, b, 10);
  bar(0.3,0.4);
  return 0;
}


void foo(int a[], int b[], int N) {
  if (a[0] > 1)
  {
    b[0] = 2;
  }
  else
  {
    b[0] = 1;
  }
  int i;
  for (i = 0; i < N; ++i) {
    #pragma unroll
    b[0]++;
  }
}
