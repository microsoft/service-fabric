// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ReplicationUnitTest
{
    class ComTestOperation : public IFabricOperationData, private Common::ComUnknownBase,  public Common::TextTraceComponent<Common::TraceTaskCodes::Replication>
    {
        DENY_COPY(ComTestOperation)

        COM_INTERFACE_LIST1(
            ComTestOperation,
            IID_IFabricOperationData,
            IFabricOperationData)

    public:
        explicit ComTestOperation(int invalidType)
            : IFabricOperationData(),
              ComUnknownBase(),
              invalidType_(invalidType),
              count_(0),
              data_()
        {
        }

        explicit ComTestOperation(int invalidType, int dataSize)
            : IFabricOperationData(),
              ComUnknownBase(),
              invalidType_(invalidType),
              count_(1),
              data_()
        {
            data_.resize(dataSize);
            replicaBuffer_[0].Buffer = data_.data();
            replicaBuffer_[0].BufferSize = static_cast<ULONG>(data_.size());
        }

        explicit ComTestOperation()
            : IFabricOperationData(),
              ComUnknownBase(),
              invalidType_(0),
              count_(0),
              data_()
        {
        }

        explicit ComTestOperation(std::wstring const & description)
            : IFabricOperationData(),
              ComUnknownBase(),
              invalidType_(0),
              count_(1),
              data_()
        {
            data_.insert(
                data_.end(),
                reinterpret_cast<BYTE const *>(description.c_str()),
                reinterpret_cast<BYTE const *>(description.c_str() + description.size() + 1));
            replicaBuffer_[0].Buffer = data_.data();
            replicaBuffer_[0].BufferSize = static_cast<ULONG>(description.size() * sizeof(wchar_t));
        }

        explicit ComTestOperation(std::wstring const & description1, std::wstring const & description2)
            : IFabricOperationData(),
              ComUnknownBase(),
              invalidType_(0),
              count_(2),
              data_()
        {
            data_.insert(
                data_.end(),
                reinterpret_cast<BYTE const *>(description1.c_str()),
                reinterpret_cast<BYTE const *>(description1.c_str() + description1.size()));
            data_.insert(
                data_.end(),
                reinterpret_cast<BYTE const *>(description2.c_str()),
                reinterpret_cast<BYTE const *>(description2.c_str() + description2.size() + 1));
            replicaBuffer_[0].Buffer = data_.data();
            replicaBuffer_[0].BufferSize = static_cast<ULONG>(description1.size() * sizeof(wchar_t));
            replicaBuffer_[1].Buffer = data_.data() + replicaBuffer_[0].BufferSize;
            replicaBuffer_[1].BufferSize = static_cast<ULONG>(description2.size() * sizeof(wchar_t));
        }

        virtual ~ComTestOperation()
        {
        }

        HRESULT STDMETHODCALLTYPE GetData(
            /*[out]*/ ULONG * count, 
            /*[out, retval]*/ FABRIC_OPERATION_DATA_BUFFER const ** buffers)
        {
            switch (invalidType_)
            {
            case 1: // zero buffer count with non-NULL buffers
                *count = 0;
                replicaBuffer_[0].Buffer = data_.data();
                replicaBuffer_[0].BufferSize = static_cast<ULONG>(data_.size());
                *buffers = &replicaBuffer_[0];
                break;

            case 2: // 1 buffer count with NULL buffers
                *count = 1;
                *buffers = NULL;
                break;
            
            case 3: // 2 buffer count with 1 NULL buffer
                *count = 2;
                replicaBuffer_[0].Buffer = data_.data();
                replicaBuffer_[0].BufferSize = static_cast<ULONG>(data_.size());
                replicaBuffer_[1].Buffer = NULL;
                replicaBuffer_[1].BufferSize = 0;
                *buffers = &replicaBuffer_[0];
                break;

            default:
                *count = count_;
                if (count_ == 0)
                {
                    *buffers = NULL;
                }
                else
                {
                    *buffers = &replicaBuffer_[0];
                }
            }
            return S_OK;
        }

    private:
        std::vector<BYTE> data_;
        ULONG count_;
        int invalidType_;
        FABRIC_OPERATION_DATA_BUFFER replicaBuffer_[2];
    };
}
