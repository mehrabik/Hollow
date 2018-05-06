#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

int main() {
	
	char username[10] = { 0 };
	char rev[10] = { 0 };
	char serial[100] = { 0 };

	printf("Please enter your username: ");
	scanf("%s", username);

	int len = strlen(username);

	if (len > 10) {
		printf("invalid username!\n");
		system("pause");
		return 0;
	}

	strcpy(rev, strrev(username));

	printf("Serial is: ");
	for(int i = 0; i < len; i++) {
		int a = rev[i] - 2*i + 100;

		printf("%d", a);

		if (i != len - 1)
			printf("-");
	}
	printf("\n");

	system("pause");
}