#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <assert.h>
#include <math.h>

static const WORD MAX_CONSOLE_LINES = 500;

char * message = "valid serial number!";

void RedirectIOToConsole()
{
	int hConHandle;
	long lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE *fp;

	// allocate a console for this app
	AllocConsole();

	// set the screen buffer to be big enough to let us scroll text
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = MAX_CONSOLE_LINES;
	SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	// redirect unbuffered STDOUT to the console
	lStdHandle = (long)GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen(hConHandle, "w");

	*stdout = *fp;

	setvbuf(stdout, NULL, _IONBF, 0);

	// redirect unbuffered STDIN to the console

	lStdHandle = (long)GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen(hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, NULL, _IONBF, 0);

	// redirect unbuffered STDERR to the console
	lStdHandle = (long)GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle(lStdHandle, _O_TEXT);

	fp = _fdopen(hConHandle, "w");

	*stderr = *fp;

	setvbuf(stderr, NULL, _IONBF, 0);
}

char** str_split(char* a_str, const char a_delim, int* count)
{
	char** result = 0;
	(*count) = 0;
	char* tmp = a_str;
	char* last_comma = 0;
	char delim[2];
	delim[0] = a_delim;
	delim[1] = 0;

	/* Count how many elements will be extracted. */
	while (*tmp)
	{
		if (a_delim == *tmp)
		{
			(*count)++;
			last_comma = tmp;
		}
		tmp++;
	}

	/* Add space for trailing token. */
	(*count) += last_comma < (a_str + strlen(a_str) - 1);

	/* Add space for terminating null string so caller
	knows where the list of returned strings ends. */
	(*count)++;

	result = (char **)malloc(sizeof(char*) * (*count));

	if (result)
	{
		size_t idx = 0;
		char* token = strtok(a_str, delim);

		while (token)
		{
			assert(idx < (*count));
			*(result + idx++) = strdup(token);
			token = strtok(0, delim);
		}
		assert(idx == (*count) - 1);
		*(result + idx) = 0;
	}

	return result;
}


int main(int argc, char** argvs) {

	RedirectIOToConsole();

	if (OpenClipboard(NULL)) {
		HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		char *pchData = (char*)GlobalLock(hClipboardData);
		char data[150];
		strcpy(data, pchData);
		GlobalUnlock(hClipboardData);
		CloseClipboard();
		
		int count = 0;
		char** args = str_split(data, ' ', &count);
		char* username = args[0];
		char* serial = args[1];

		count = 0;
		char** serialparts = str_split(serial, '-', &count);
		count--;
		int len = strlen(username);
		char *result = new char[count + 1];

		if (count == 0 || count != strlen(username)) {
			printf("\n");
			printf("In");
			printf(message);
			printf("\n");
			exit(0);
		}

		int i = 0;
		for (i = 0; i < count; i++) {
			
			if (i > len) {
				printf("\n");
				printf("In");
				printf(message);
				printf("\n");
				exit(0);
			}

			int a = atoi(serialparts[i]);
			
			if (a == 0) {
				printf("\n");
				printf("In");
				printf(message);
				printf("\n");
				exit(0);
			}

			a = a + 2 * i - 100;
			result[i] = a;
		}

		result[i] = 0;
		
		char* rev = new char[strlen(result) + 1];
		rev = strrev(result);

		if (strcmp(rev, username) == 0) {
			printf("\n");
			printf(message);
			printf("\n");
			exit(0);
		}
		else {
			printf("\n");
			printf("In");
			printf(message);
			printf("\n");
			exit(0);
		}
	}
	else {
		printf("\n");
		printf("In");
		printf(message);
		printf("\n");
		exit(0);
	}

	return 0;
}