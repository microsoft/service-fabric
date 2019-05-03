// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace std;

namespace Common
{
    AsyncOperationSPtr AsyncFile::BeginReadFile(
        __in std::wstring const &filePath,
        __in TimeSpan const timeout,
        __in AsyncCallback const& callback,
        __in AsyncOperationSPtr const& parent)
    {
        return AsyncOperation::CreateAndStart<ReadFileAsyncOperation>(filePath, timeout, callback, parent);
    }

    ErrorCode AsyncFile::EndReadFile(
        __in AsyncOperationSPtr const& operation,
        __out ByteBuffer &buffer)
    {
        return AsyncFile::ReadFileAsyncOperation::End(operation, buffer);
    }
    
    AsyncOperationSPtr AsyncFile::BeginWriteFile(
        __in HANDLE const&  fileHandle,
        __in ByteBuffer inputBuffer,
        __in TimeSpan const timeout,
        __in AsyncCallback const& callback,
        __in AsyncOperationSPtr const& parent)
    {
        return AsyncOperation::CreateAndStart<WriteFileAsyncOperation>(fileHandle, inputBuffer, timeout, callback, parent);
    }

    ErrorCode AsyncFile::EndWriteFile(
        __in AsyncOperationSPtr const& operation, __out DWORD & winError)
    {
        return AsyncFile::WriteFileAsyncOperation::End(operation, winError);
    }
}
