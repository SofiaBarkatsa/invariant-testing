// RUN: %crabllvm -O0 --lower-unsigned-icmp --crab-dom=term-dis-int --crab-track=arr --crab-check=assert "%s" 2>&1 | OutputCheck %s
// CHECK: ^0  Number of total error checks$
// CHECK: ^1  Number of total warning checks$
extern int nd ();
extern void __CRAB_assert(int);

int a[10];

int main ()
{
  int i;
  for (i=0;i<10;i++)
  {
    if (nd ())
      a[i] =0;
    else 
      a[i] =5;
  }

  int res = a[i-1];
  __CRAB_assert(res == 4); 
  return res;
}
