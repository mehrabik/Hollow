#include <Windows.h>
#include <stdio.h>

typedef NTSTATUS(__stdcall* pNtUnmapViewOfSection)(HANDLE ProcessHandle, PVOID BaseAddress);

typedef struct {
	PIMAGE_DOS_HEADER dos_header;
	PIMAGE_NT_HEADERS nt_headers;
	PIMAGE_SECTION_HEADER section_header;
	LPBYTE file_data;
} NEW_PROCESS_INFO, *PNEW_PROCESS_INFO;

#pragma once
