#include<stdio.h>

int main()
{
	int a=100;
    int b,c;
	int d=0;
	
	while(a!=1)
	{
		for(int b=a-1;b!=1;b=b-1)
	{
		c=a%b;
		if(c==0)
		{
			d=1;
			break;	
		}	
	}
	
	if(d!=1)
	{
		printf("%d ",a);
	}
	d=0;
	a=a-1;
	}
	
	return 0;
}