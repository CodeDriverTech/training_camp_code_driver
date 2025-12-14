#include<stdio.h>

int main()
{
	int a;
	int b=0;
	double c=0;
	double d=0;
	
	scanf("%d",&a);
	
	while(a!=-1)
	{
		b=b+1;
		c=c+a;
		scanf("%d",&a);
	}
	
	d=c/b;
	
	printf("%lf\n",d);
	
	return 0;
	
}