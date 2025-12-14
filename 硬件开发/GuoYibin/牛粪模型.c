#include<stdio.h>
#include<stdlib.h>    

int main()
{
	int min_1,min_2,x,y,a,b;
	
	scanf("%d %d %d %d",&a,&b,&x,&y);
	
	min_1=abs(a-b);
	
	if(abs(a-x)>abs(a-y)){
		min_2=abs(a-x)+abs(b-y);
	}
	else{
		min_2=abs(a-y)+abs(b-x);
	}
	printf("%d",min_1<min_2?min_1:min_2);
	
	return 0;
}