// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ComReplicatedOperationData : 
        public IFabricOperationData, 
        public Common::ComUnknownBase
    {
        COM_INTERFACE_LIST1(
            ComReplicatedOperationData,
            IID_IFabricOperationData,
            IFabricOperationData)

    public:
        ComReplicatedOperationData(Serialization::IFabricSerializableStreamUPtr streamUPtr);
            
        __declspec(property(get=get_IsEmpty)) bool IsEmpty;
        bool get_IsEmpty() { return totalSize_ == 0; }

        __declspec(property(get=get_OperationDataSize)) ULONG OperationDataSize;
        ULONG get_OperationDataSize() { return totalSize_; }

        __declspec(property(get=get_BufferCount)) size_t BufferCount;
        size_t get_BufferCount() { return buffers_.size(); }

        HRESULT STDMETHODCALLTYPE GetData(__out ULONG * count, __out FABRIC_OPERATION_DATA_BUFFER const ** buffers);
    private:
        void InitBuffers();
        
        // We should keep the serialized stream pointer alive, until the operation is released by the replicator 
        Serialization::IFabricSerializableStreamUPtr streamUPtr_;
        Common::ErrorCode initialized_;
        ULONG totalSize_;
        std::vector<FABRIC_OPERATION_DATA_BUFFER> buffers_;
    };
}
