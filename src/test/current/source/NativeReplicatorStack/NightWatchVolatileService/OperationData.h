// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace V1ReplPerf
{
    class OperationData : public IFabricOperationData, Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(OperationData)
            COM_INTERFACE_LIST1(
                FabricOperationData,
                IID_IFabricOperationData,
                IFabricOperationData)
    public:

        OperationData()
            : value_()
        {
        }

        OperationData(int valueSize)
            : value_(valueSize)
        { 
            for (int i = 0; i < value_; i++)
            {
                buffer_.push_back(static_cast<byte>(0));
            }
        }

        HRESULT STDMETHODCALLTYPE GetData(
            /*[out]*/ ULONG * count,
            /*[out, retval]*/ FABRIC_OPERATION_DATA_BUFFER const ** buffers)
        {
            *count = 1;
            
            replicaBuffer_.BufferSize = static_cast<ULONG>(value_);
            replicaBuffer_.Buffer = buffer_.data();

            *buffers = &replicaBuffer_;

            return S_OK;
        }

    private:
        int value_;
        std::vector<byte> buffer_;
        FABRIC_OPERATION_DATA_BUFFER replicaBuffer_;
    };

}

