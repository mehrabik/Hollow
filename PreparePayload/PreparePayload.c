#include "PreparePayload.h"
#include "aes.h"

#include <Windows.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

int main(int argc, char** argvs) {
	
	//check arguments
	if (argc <= 2) {
		printf("Usage: PreparePayload <PayloadFile> <MainFile>\n");
		system("pause");
		return 0;
	}

	//read arguments
	char* payload_file_path = argvs[1];
	char* main_file_path = argvs[2];

	//read payload file
	HANDLE hfile = CreateFileA(payload_file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == NULL) {
		DWORD dw = GetLastError();
		printf("Error openning payload file: %d\n", dw);
		system("pause");
		return -3;
	}
	DWORD file_size = GetFileSize(hfile, NULL); //Note: High DWORD ignored, dangerous with >4GB files
	LPBYTE file_data = (LPBYTE)malloc(file_size * sizeof(LPBYTE));
	DWORD bytes_read;
	ReadFile(hfile, file_data, file_size, &bytes_read, 0);
	assert(bytes_read == file_size);

	//encrypt payload file data
	LPBYTE encrypted_file_data = (LPBYTE)malloc(file_size * sizeof(LPBYTE));
	int key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	int iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };
	int idx = 0;
	while (1) {
		AES128_CBC_encrypt_buffer(encrypted_file_data + idx, file_data + idx, 16, key, iv);
		idx = idx + 16;

		if (idx > bytes_read) {
			idx = idx - 16;
			int remained = bytes_read - idx;
			AES128_CBC_encrypt_buffer(encrypted_file_data + idx, file_data + idx, remained, key, iv);
			break;
		}
	}

	//write to main file's resources
	HANDLE hmainfile = BeginUpdateResourceA(main_file_path, FALSE);
	if (hmainfile == NULL) {
		DWORD dw = GetLastError();
		printf("Error openning main file: %d\n", dw);
		system("pause");
		return -1;
	}
	BOOL result = UpdateResource(hmainfile, MAKEINTRESOURCE(10), TEXT("DATA"), 0, encrypted_file_data, bytes_read);
	if (result == FALSE) {
		DWORD dw = GetLastError();
		printf("Error updating resource: %d\n", dw);
		system("pause");
		return -2;
	}
	if (!EndUpdateResource(hmainfile, FALSE)) {
		DWORD dw = GetLastError();
		printf("Error writing resource: %d\n", dw);
		system("pause");
		return -4;

	}

	//finish
	printf("Success!\n");
	return 0;
}