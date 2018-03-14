// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "ikbase.h"

namespace Serialization
{
    __interface IFabricStream : public IKBase
    {
        virtual NTSTATUS WriteBytes(__in ULONG size, __in VOID const * source) = 0;

        virtual NTSTATUS WriteBytesNoCopy(__in ULONG size, __in VOID const * source) = 0;

        virtual NTSTATUS ReadBytes(__in ULONG size, _Out_writes_(size) VOID * destination) = 0;

        virtual NTSTATUS ReadBytesNoCopy(__in ULONG index, __out FabricIOBuffer & buffer);

        virtual NTSTATUS GetNextBuffer(__out FabricIOBuffer & buffer, __out bool & isExtensionData) = 0;

        virtual NTSTATUS GetAllBuffers(_Out_ FabricIOBuffer* bufferArray, _Inout_ size_t & count) const = 0;

        virtual NTSTATUS Seek(__in ULONG position) = 0;

        virtual NTSTATUS SeekToEnd() = 0;

        virtual ULONG get_Position() const = 0;

        // Returns the current size of the stream, i.e. the total max bytes 
        // written so far, irrespective of internal allocation sizes.
        virtual ULONG Size() const = 0;

        virtual NTSTATUS AddCompletionCallback(__in FabricCompletionCallback completionCallback, __in VOID * state) = 0;

        virtual NTSTATUS InvokeCompletionCallbacks() = 0;
    };

    typedef KUniquePtr<IFabricStream, IKBaseDeleter<IFabricStream>> IFabricStreamUPtr;
}
