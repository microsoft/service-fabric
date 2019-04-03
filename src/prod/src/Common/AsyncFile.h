// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class AsyncFile
    {

    public:
        static AsyncOperationSPtr BeginReadFile(
            __in std::wstring const &filePath,
            __in TimeSpan const timeout,
            __in AsyncCallback const& callback,
            __in AsyncOperationSPtr const& parent);

        static ErrorCode EndReadFile(
            __in AsyncOperationSPtr const& operation,
            __out ByteBuffer &buffer);

        static AsyncOperationSPtr BeginWriteFile(
            __in HANDLE const& fileHandle,
            __in ByteBuffer inputBuffer,
            __in TimeSpan const timeout,
            __in AsyncCallback const& callback,
            __in AsyncOperationSPtr const& parent);

        static ErrorCode EndWriteFile(
            __in AsyncOperationSPtr const& operation, __out DWORD & winError);

        class ReadFileAsyncOperation;
        class WriteFileAsyncOperation;
    };

}
