#include<stdio.h>

int main()
{
	int a,n,m,x;
	
	scanf("%d",&a);
	m=1;
	
	while(m<=a){
		n=1;
		while(n<=m){
			x=n*m;
			if(n>1){
				printf(" ");
			}
			printf("%d*%d=%d ",n,m,x);
			n=n+1;
			if(x<10){
				printf(" ");
			}
		}
	m=m+1;
	printf("\n");
	}
	
	return 0;
}