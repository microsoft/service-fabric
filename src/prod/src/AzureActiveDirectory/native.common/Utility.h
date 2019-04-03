// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DllExport __declspec( dllexport )

#if DotNetCoreClrIOT

#define UNREFERENCED_PARAMETER(P) (P)
#define CHECK_NULL(param) \
if (param == nullptr) \
{ \
    return 1; \
} 

#else

#define CHECK_NULL_PARAM(param, buffer, size) \
if (param == nullptr) \
{ \
    CopyFromManaged(buffer, size, "[null param]"); \
    return 1; \
} \

#define CHECK_NULL(param) CHECK_NULL_PARAM(param, errorMessageBuffer, errorMessageBufferSize)
#define CHECK_NULL_OUT(param) CHECK_NULL_PARAM(param, outBuffer, outBufferSize)

void CopyFromManaged(wchar_t *, int, System::String^);

#endif