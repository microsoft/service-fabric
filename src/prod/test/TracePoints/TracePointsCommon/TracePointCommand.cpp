// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TracePoints;

bool TracePointCommand::TryCreate(LPCWSTR dllName, LPCSTR functionName, LPCWSTR userData, BYTE opCode, TracePointCommand::Data & command)
{
    size_t dllNameLength = wcslen(dllName);
    size_t functionNameLength = strlen(functionName);
    size_t userDataLength = wcslen(userData);

    if (dllNameLength > TracePointCommand::MaxDllNameLength ||
        functionNameLength > TracePointCommand::MaxDllNameLength ||
        userDataLength > TracePointCommand::MaxUserDataLength)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return false;
    }

    errno_t error = wcscpy_s(command.DllName, dllNameLength + 1, dllName);
    if (error != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    error = strncpy_s(command.FunctionName, functionName, functionNameLength + 1);
    if (error != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    error = wcscpy_s(command.UserData, userDataLength + 1, userData);
    if (error != 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return false;
    }

    command.OpCode = opCode;

    SetLastError(ERROR_SUCCESS);
    return true;
}
