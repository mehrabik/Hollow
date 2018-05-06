#include "HollowMain.h"
#include "aes.h"
#include <assert.h>

void get_replacement_info(PNEW_PROCESS_INFO new_process_info) {

	//reading resrouce data
	HMODULE hself = GetModuleHandle(NULL);
	HRSRC hresinfo = FindResource(hself, TEXT("DATA"), MAKEINTRESOURCE(10));
	if (hresinfo == NULL) {
		DWORD dw = GetLastError();
		//printf("Can not file resource: %d\n");
		//system("pause");
		exit(-1);
	}
	HGLOBAL hres = LoadResource(hself, hresinfo);
	if (hres == NULL) {
		DWORD dw = GetLastError();
		//printf("Can not load resource: %d\n");
		//system("pause");
		exit(-2);
	}
	LPVOID encrypted_file_data = LockResource(hres);
	DWORD encrypted_file_size = SizeofResource(hself, hresinfo);

	//decrypting data
	LPBYTE file_data = (LPBYTE)malloc(encrypted_file_size * sizeof(LPBYTE));
	int key[] = { 0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6, 0xab, 0xf7, 0x15, 0x88, 0x09, 0xcf, 0x4f, 0x3c };
	int iv[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f };

	int idx = 0;
	while (1) {
		AES128_CBC_decrypt_buffer(file_data + idx, (LPBYTE)encrypted_file_data + idx, 16, key, iv);
		idx = idx + 16;

		if (idx > encrypted_file_size) {
			idx = idx - 16;
			int remained = encrypted_file_size - idx;
			AES128_CBC_decrypt_buffer(file_data + idx, (LPBYTE)encrypted_file_data + idx, remained, key, iv);
			break;
		}
	}

	//preparing process info
	new_process_info->file_data = (LPBYTE)file_data;
	DWORD bytes_read;

	new_process_info->dos_header =
		(PIMAGE_DOS_HEADER)(&new_process_info->file_data[0]);
	new_process_info->nt_headers =
		(PIMAGE_NT_HEADERS)(&new_process_info->file_data[new_process_info->dos_header->e_lfanew]);
}

int main(int argc, char* argv[]) {

	//start up
	NEW_PROCESS_INFO new_process_info;
	PROCESS_INFORMATION process_info;
	STARTUPINFOA startup_info;
	RtlZeroMemory(&startup_info, sizeof(STARTUPINFOA));
	pNtUnmapViewOfSection NtUnmapViewOfSection = NULL;
	system("title Hollow");

	//get user input
	char username[10] = { 0 };
	char serial[50] = { 0 };

	printf("\nWelcome...\n");
	printf("Please enter your name: ");
	scanf("%s", username);

	printf("Please enter your serial: ");
	scanf("%s", serial);

	char commandline[150];
	strcpy(commandline, username);
	strcat(commandline, " ");
	strcat(commandline, serial);

	//copy data to clipboard
	if (OpenClipboard(NULL)) {
		EmptyClipboard();
		HGLOBAL hclipboarddata = GlobalAlloc(GMEM_DDESHARE, strlen(commandline) + 1);
		char* pchdata = (char*)GlobalLock(hclipboarddata);
		strcpy(pchdata, commandline);
		GlobalUnlock(hclipboarddata);
		SetClipboardData(CF_TEXT, hclipboarddata);
		CloseClipboard();
	}
	else {
		//printf("Failed to open clipboard: %d\n");
		//system("pause");
		exit(-1);
	}
	
	//finding current process filename
	HMODULE hself = GetModuleHandle(NULL);
	WCHAR selfpath[MAX_PATH];
	GetModuleFileName(hself, selfpath, MAX_PATH);

	//create process
	if (!CreateProcess(selfpath, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED,
		NULL, NULL, &startup_info, &process_info)) {
		DWORD dw = GetLastError();
		//printf("Failed to create process: %d\n", dw);
		exit(-1);
	}

	//get replacement data
	get_replacement_info(&new_process_info);

	//dynamic api loading
	NtUnmapViewOfSection = (pNtUnmapViewOfSection)(GetProcAddress(
		GetModuleHandleA("ntdll.dll"), "NtUnmapViewOfSection"));

	//Remove target memory code
	NtUnmapViewOfSection(process_info.hProcess, (PVOID)new_process_info.nt_headers->OptionalHeader.ImageBase);

	//Allocate memory in target process starting at replacements image base
	VirtualAllocEx(process_info.hProcess, (PVOID)new_process_info.nt_headers->OptionalHeader.ImageBase,
		new_process_info.nt_headers->OptionalHeader.SizeOfImage,
		MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	//Copy in PE header of replacement process
	WriteProcessMemory(process_info.hProcess, (PVOID)new_process_info.nt_headers->OptionalHeader.ImageBase,
		&new_process_info.file_data[0], new_process_info.nt_headers->OptionalHeader.SizeOfHeaders, NULL);

	//Write in all sections of the replacement process
	for (int i = 0; i < new_process_info.nt_headers->FileHeader.NumberOfSections; i++) {
		//Get offset of section
		int section_offset = new_process_info.dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS) +
			(sizeof(IMAGE_SECTION_HEADER) * i);
		new_process_info.section_header = (PIMAGE_SECTION_HEADER)(&new_process_info.file_data[section_offset]);
		//Write in section
		WriteProcessMemory(process_info.hProcess, (PVOID)(new_process_info.nt_headers->OptionalHeader.ImageBase +
			new_process_info.section_header->VirtualAddress),
			&new_process_info.file_data[new_process_info.section_header->PointerToRawData],
			new_process_info.section_header->SizeOfRawData, NULL);
	}

	//Get CONTEXT of main thread of suspended process, fix up EAX to point to new entry point
	LPCONTEXT thread_context = (LPCONTEXT)_aligned_malloc(sizeof(CONTEXT), sizeof(DWORD));
	thread_context->ContextFlags = CONTEXT_FULL;
	GetThreadContext(process_info.hThread, thread_context);
	thread_context->Eax = new_process_info.nt_headers->OptionalHeader.ImageBase +
		new_process_info.nt_headers->OptionalHeader.AddressOfEntryPoint;
	SetThreadContext(process_info.hThread, thread_context);

	//Resume the main thread, now holding the replacement processes code
	ResumeThread(process_info.hThread);

	//clean up
	free(new_process_info.file_data);
	_aligned_free(thread_context);
	return 0;
}
