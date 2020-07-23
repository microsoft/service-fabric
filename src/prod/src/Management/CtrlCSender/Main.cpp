// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
	if (argc < 2)
	{
		return -1;
	}

	wchar_t *end;
	DWORD processId = ::wcstoul(argv[1], &end, 10);
	if (*end != 0)
	{
		return -2;
	}

	if (!::FreeConsole())
	{
		DWORD error = ::GetLastError();
		if (error != ERROR_INVALID_PARAMETER)
		{
			return error;
		}
	}

	if (!::AttachConsole(processId))
	{
		return ::GetLastError();
	}

	if (!::GenerateConsoleCtrlEvent(CTRL_C_EVENT, processId))
	{
		return ::GetLastError();
	}

	return 0;
}
