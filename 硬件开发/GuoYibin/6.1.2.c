#include<stdio.h>
#include<math.h>

int main()
{
	int n,a,x,y,z;
	
	scanf("%d",&n);
	
	a=pow(10,n-1);
	
	while(a<pow(10,n)){
		 x=0;
		 y=a;
		 while(y!=0){
		 	z=y%10;
		 	x=x+pow(z,n);
		 	y=y/10;
		 }
		 if(x==a){
		 	printf("%d\n",x);
		 }
		 a=a+1;
	}
	
	return 0;
}