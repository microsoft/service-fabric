// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TracePoints
{
    namespace TracePointCommand
    {
        BYTE const SetupOpCode = 0;
        BYTE const InvokeOpCode = 1;
        BYTE const CleanupOpCode = 2;

        size_t const MaxDllNameLength = MAX_PATH;
        size_t const MaxFunctionNameLength = MAX_PATH;
        size_t const MaxUserDataLength = MAX_PATH;

        struct Data
        {
            WCHAR DllName[MaxDllNameLength + 1];
            CHAR FunctionName[MaxFunctionNameLength + 1];
            WCHAR UserData[MaxUserDataLength + 1];
            BYTE OpCode;
        };

        bool TryCreate(LPCWSTR dllName, LPCSTR functionName, LPCWSTR userData, BYTE opCode, Data & command);
    }
}
