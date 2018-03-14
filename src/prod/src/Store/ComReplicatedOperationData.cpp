// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using Common::ComUtility;
using Common::ErrorCode;
using Common::FabricSerializer;

namespace Store
{
    ComReplicatedOperationData::ComReplicatedOperationData(
        Serialization::IFabricSerializableStreamUPtr streamUPtr)
        : streamUPtr_(Ktl::Move(streamUPtr))
        , initialized_()
        , totalSize_(0)
        , buffers_()
    {
        InitBuffers();
    }

    void ComReplicatedOperationData::InitBuffers()
    {
        ErrorCode error;

        if (streamUPtr_)
        {
            error = FabricSerializer::GetOperationBuffersFromSerializableStream(
                streamUPtr_,
                buffers_,
                totalSize_);
        }

        // We consider streamUPtr_ = NULL as success to represent an empty operation
        initialized_ = error;
    }
       
    HRESULT STDMETHODCALLTYPE ComReplicatedOperationData::GetData(__out ULONG * count, __out FABRIC_OPERATION_DATA_BUFFER const ** buffers)
    {
        if (count == NULL || buffers == NULL)
        {
            return ComUtility::OnPublicApiReturn(E_POINTER);
        }

        if (initialized_.IsSuccess())
        {
            *count = static_cast<ULONG>(buffers_.size());
            *buffers = buffers_.data();
        }

        return ComUtility::OnPublicApiReturn(initialized_.ToHResult());
    }    
}
