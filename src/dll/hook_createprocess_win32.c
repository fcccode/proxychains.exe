// SPDX-License-Identifier: GPL-2.0-or-later
/* hook_createprocess_win32.c
 * Copyright (C) 2020 Feng Shun.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as 
 *   published by the Free Software Foundation, either version 3 of the
 *   License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License version 2 for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   version 2 along with this program. If not, see
 *   <http://www.gnu.org/licenses/>.
 */
#include "common_win32.h"
#include "hookdll_interior_win32.h"
#include "log_win32.h"

#include "hookdll_win32.h"

PROXY_FUNC(CreateProcessA)
{
	BOOL bRet;
	DWORD dwErrorCode;
	DWORD dwReturn;
	PROCESS_INFORMATION processInformation;

	bRet = orig_fpCreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, &processInformation);
	dwErrorCode = GetLastError();

	if (lpProcessInformation) {
		CopyMemory(lpProcessInformation, &processInformation, sizeof(PROCESS_INFORMATION));
	}

	LOGD(L"CreateProcessA: " WPRS L", " WPRS, lpApplicationName, lpCommandLine);

	if (!bRet) goto err_orig;

	dwReturn = InjectTargetProcess(&processInformation);
	if (!(dwCreationFlags & CREATE_SUSPENDED)) {
		ResumeThread(processInformation.hThread);
	}
	//if (GetCurrentProcessId() != g_pPxchConfig->dwMasterProcessId) IpcCommunicateWithServer();
	if (dwReturn != 0) goto err_inject;
	return 1;

err_orig:
	LOGE(L"CreateProcessA Error: " WPRDW L", %ls", bRet, FormatErrorToStr(dwErrorCode));
	SetLastError(dwErrorCode);
	return bRet;

err_inject:
	// PrintErrorToFile(stderr, dwReturn);
	SetLastError(dwReturn);
	return 1;
}

PROXY_FUNC(CreateProcessW)
{
	BOOL bRet;
	DWORD dwErrorCode;
	DWORD dwReturn = 0;
	PROCESS_INFORMATION processInformation;

	g_bCurrentlyInWinapiCall = TRUE;

	// For cygwin: cygwin fork() will duplicate the data in child process, including pointer g_*.
	RestoreChildData();

	IPCLOGD(L"(In CreateProcessW) g_pRemoteData->dwDebugDepth = " WPRDW, g_pRemoteData ? g_pRemoteData->dwDebugDepth : -1);

	IPCLOGD(L"CreateProcessW: %ls, %ls, lpProcessAttributes: %#llx, lpThreadAttributes: %#llx, bInheritHandles: %d, dwCreationFlags: %#lx, lpCurrentDirectory: %s", lpApplicationName, lpCommandLine, (UINT64)lpProcessAttributes, (UINT64)lpThreadAttributes, bInheritHandles, dwCreationFlags, lpCurrentDirectory);

	bRet = orig_fpCreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, &processInformation);
	dwErrorCode = GetLastError();

	IPCLOGV(L"CreateProcessW: Created.(%u) Child process id: " WPRDW, bRet, processInformation.dwProcessId);

	if (lpProcessInformation) {
		CopyMemory(lpProcessInformation, &processInformation, sizeof(PROCESS_INFORMATION));
	}

	IPCLOGV(L"CreateProcessW: Copied.");
	if (!bRet) goto err_orig;
	
	IPCLOGV(L"CreateProcessW: After jmp to err_orig.");
	IPCLOGV(L"CreateProcessW: Before InjectTargetProcess.");
	dwReturn = InjectTargetProcess(&processInformation);

	IPCLOGV(L"CreateProcessW: Injected. " WPRDW, dwReturn);

	if (!(dwCreationFlags & CREATE_SUSPENDED)) {
		ResumeThread(processInformation.hThread);
	}

	if (dwReturn != 0) goto err_inject;
	IPCLOGD(L"I've Injected WINPID " WPRDW, processInformation.dwProcessId);

	g_bCurrentlyInWinapiCall = FALSE;
	return 1;

err_orig:
	IPCLOGE(L"CreateProcessW Error: " WPRDW L", %ls", bRet, FormatErrorToStr(dwErrorCode));
	SetLastError(dwErrorCode);
	g_bCurrentlyInWinapiCall = FALSE;
	return bRet;

err_inject:
	IPCLOGE(L"Injecting WINPID " WPRDW L" Error: %ls", processInformation.dwProcessId, FormatErrorToStr(dwReturn));
	SetLastError(dwReturn);
	g_bCurrentlyInWinapiCall = FALSE;
	return 1;
}

PROXY_FUNC(CreateProcessAsUserW)
{
	BOOL bRet;
	DWORD dwErrorCode;
	DWORD dwReturn = 0;
	PROCESS_INFORMATION processInformation;

	g_bCurrentlyInWinapiCall = TRUE;

	// For cygwin: cygwin fork() will duplicate the data in child process, including pointer g_*.
	RestoreChildData();

	IPCLOGI(L"(In CreateProcessAsUserW) g_pRemoteData->dwDebugDepth = " WPRDW, g_pRemoteData ? g_pRemoteData->dwDebugDepth : -1);

	IPCLOGD(L"CreateProcessAsUserW: %ls, %ls, lpProcessAttributes: %#llx, lpThreadAttributes: %#llx, bInheritHandles: %d, dwCreationFlags: %#lx, lpCurrentDirectory: %s", lpApplicationName, lpCommandLine, (UINT64)lpProcessAttributes, (UINT64)lpThreadAttributes, bInheritHandles, dwCreationFlags, lpCurrentDirectory);

	bRet = orig_fpCreateProcessAsUserW(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags | CREATE_SUSPENDED, lpEnvironment, lpCurrentDirectory, lpStartupInfo, &processInformation);
	dwErrorCode = GetLastError();

	IPCLOGV(L"CreateProcessAsUserW: Created.(%u) Child process id: " WPRDW, bRet, processInformation.dwProcessId);

	if (lpProcessInformation) {
		CopyMemory(lpProcessInformation, &processInformation, sizeof(PROCESS_INFORMATION));
	}

	IPCLOGV(L"CreateProcessAsUserW: Copied.");
	if (!bRet) goto err_orig;

	IPCLOGV(L"CreateProcessAsUserW: After jmp to err_orig.");
	IPCLOGV(L"CreateProcessAsUserW: Before InjectTargetProcess.");
	dwReturn = InjectTargetProcess(&processInformation);

	IPCLOGV(L"CreateProcessAsUserW: Injected. " WPRDW, dwReturn);

	if (!(dwCreationFlags & CREATE_SUSPENDED)) {
		ResumeThread(processInformation.hThread);
	}

	if (dwReturn != 0) goto err_inject;
	IPCLOGD(L"CreateProcessAsUserW: I've Injected WINPID " WPRDW, processInformation.dwProcessId);

	g_bCurrentlyInWinapiCall = FALSE;
	return 1;

err_orig:
	IPCLOGE(L"CreateProcessAsUserW Error: " WPRDW L", %ls", bRet, FormatErrorToStr(dwErrorCode));
	SetLastError(dwErrorCode);
	g_bCurrentlyInWinapiCall = FALSE;
	return bRet;

err_inject:
	IPCLOGE(L"Injecting WINPID " WPRDW L" Error: %ls", processInformation.dwProcessId, FormatErrorToStr(dwReturn));
	SetLastError(dwReturn);
	g_bCurrentlyInWinapiCall = FALSE;
	return 1;
}
