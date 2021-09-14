//是循环变量bit16的问题
#include "typedefs.h"

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


void foo(int a[], int b[], int N, bit2 flag, bit8 max_min[], bit16 max_index[], Triangle_2D triangle_2d_same, CandidatePixel fragment2[]) {
  if (a[0] > 1)
  {
    b[0] = 2;
  }
  else
  {
    b[0] = 1;
  }

  for (int i = 0; i < N; ++i) {
    b[0]++;
  }
}

bit16 rasterization2 ( bit2 flag, bit8 max_min[], bit16 max_index[], Triangle_2D triangle_2d_same, CandidatePixel fragment2[] )
{
  #pragma HLS INLINE off
  // clockwise the vertices of input 2d triangle
  if ( flag )
  {
    return 0;
  }
  bit8 color = 100;
  bit16 i = 0;
  int max = max_index[0];

  //RAST2: 
  for ( int k = 0; k < max; k++ )
  {
    #pragma HLS PIPELINE II=1
    bit8 x = max_min[0] + k%max_min[4];
    bit8 y = max_min[2] + k/max_min[4];

    if( pixel_in_triangle( x, y, triangle_2d_same ) )
    {
      fragment2[i].x = x;
      fragment2[i].y = y;
      fragment2[i].z = triangle_2d_same.z;
      fragment2[i].color = color;
      i++;
    }
  }

  return i;
}