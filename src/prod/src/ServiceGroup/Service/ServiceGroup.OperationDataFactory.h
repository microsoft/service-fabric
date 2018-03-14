// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Reliability/Replication/IOperationDataFactory.h"

using namespace Reliability::ReplicationComponent;

namespace ServiceGroup
{
    class COperationDataFactory : 
        public IFabricOperationData, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
            DENY_COPY(COperationDataFactory)
        
        public:
            //
            // Constructor/Destructor.
            //
            COperationDataFactory(void) 
            {
                WriteNoise(TraceSubComponentOperationDataFactory, "this={0} - ctor", this);

                replicaBuffers_ = NULL;
                count_ = 0;
            }

            virtual ~COperationDataFactory(void)
            {
                WriteNoise(TraceSubComponentOperationDataFactory, "this={0} - dtor", this);

                if (NULL != replicaBuffers_)
                {
                     for (ULONG index = 0; index < count_; index++)
                     {
                         delete[] replicaBuffers_[index].Buffer;
                     }
                     delete[] replicaBuffers_;
                }
            }
            
            BEGIN_COM_INTERFACE_LIST(COperationDataFactory)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
                COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
            END_COM_INTERFACE_LIST()
            
            //
            // IFabricOperationData methods.
            //

            STDMETHOD(GetData)(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers)
            {
                ASSERT_IF(NULL == count, "Unexpected length");
                ASSERT_IF(NULL == buffers, "Unexpected buffer");

                if (NULL != replicaBuffers_)
                {
                    *count = count_;
                    *buffers = replicaBuffers_;
                }
                else
                {
                    *count = 0;
                    *buffers = NULL;
                }

                return S_OK;
            }
            //
            // Initialization.
            //
            STDMETHOD(FinalConstruct)(
                __in ULONG segmentSizesCount,
                __in_ecount( segmentSizesCount ) ULONG * segmentSizes)
            {
                ASSERT_IF(0 == segmentSizesCount, "Unexpected length");

                WriteNoise(TraceSubComponentOperationDataFactory, "this={0} - Constructing {1} buffers", this, segmentSizesCount);

                replicaBuffers_ = new (std::nothrow) FABRIC_OPERATION_DATA_BUFFER[segmentSizesCount];
                if (NULL == replicaBuffers_)
                {
                    ServiceGroupReplicationEventSource::GetEvents().Error_0_OperationDataFactory(reinterpret_cast<uintptr_t>(this));
                    return E_OUTOFMEMORY;
                }
                for (ULONG index = 0; index < segmentSizesCount; index++)
                {
                    replicaBuffers_[index].BufferSize = 0;
                    replicaBuffers_[index].Buffer = NULL;
                }
                HRESULT hr = S_OK;
                for (ULONG index = 0; index < segmentSizesCount; index++)
                {
                    replicaBuffers_[index].Buffer = new (std::nothrow) BYTE[segmentSizes[index]];
                    if (NULL == replicaBuffers_[index].Buffer)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    replicaBuffers_[index].BufferSize = segmentSizes[index];

                    WriteNoise(TraceSubComponentOperationDataFactory, "this={0} - Constructing buffer {1} of size {2}", this, index, segmentSizes[index]);
                }
                if (FAILED(hr))
                {

                    for (ULONG index = 0; index < segmentSizesCount; index++)
                    {
                        if (0 != replicaBuffers_[index].BufferSize)
                        {
                            delete[] replicaBuffers_[index].Buffer;
                        }
                    }
                    delete[] replicaBuffers_;
                    replicaBuffers_ = NULL;

                    ServiceGroupReplicationEventSource::GetEvents().Error_0_OperationDataFactory(reinterpret_cast<uintptr_t>(this));
                    return hr;
                }

                WriteNoise(TraceSubComponentOperationDataFactory, "this={0} - Succeeded", this);
                count_ = segmentSizesCount;
                return S_OK;
            }

            //
            // Initialization.
            //
            // This does a mem cpy and stores the buffers
            STDMETHOD(FinalConstruct)(
                ULONG bufferCount,
                FABRIC_OPERATION_DATA_BUFFER_EX1 const * buffers)
            {
                ASSERT_IF(0 == bufferCount, "Unexpected buffer Count");

                WriteNoise(TraceSubComponentOperationDataFactory, "this={0} - Constructing {1} buffers from ex1", this, bufferCount);

                replicaBuffers_ = new (std::nothrow) FABRIC_OPERATION_DATA_BUFFER[bufferCount];
                if (NULL == replicaBuffers_)
                {
                    ServiceGroupReplicationEventSource::GetEvents().Error_0_OperationDataFactory(reinterpret_cast<uintptr_t>(this));
                    return E_OUTOFMEMORY;
                }
                for (ULONG index = 0; index < bufferCount; index++)
                {
                    replicaBuffers_[index].BufferSize = 0;
                    replicaBuffers_[index].Buffer = NULL;
                }
                HRESULT hr = S_OK;
                for (ULONG index = 0; index < bufferCount; index++)
                {
                    replicaBuffers_[index].Buffer = new (std::nothrow) BYTE[buffers[index].Count];
                    if (NULL == replicaBuffers_[index].Buffer)
                    {
                        hr = E_OUTOFMEMORY;
                        break;
                    }
                    replicaBuffers_[index].BufferSize = buffers[index].Count;

                    auto err = memcpy_s(
                        replicaBuffers_[index].Buffer,
                        buffers[index].Count,
                        buffers[index].Buffer + buffers[index].Offset,
                        buffers[index].Count);

                    if (err != ERROR_SUCCESS)
                    {
                        hr = err;
                        break;
                    }

                    WriteNoise(TraceSubComponentOperationDataFactory, "this={0} - Constructing buffer {1} of size {2}", this, index, buffers[index].Count);
                }
                if (FAILED(hr))
                {
                    for (ULONG index = 0; index < bufferCount; index++)
                    {
                        if (0 != replicaBuffers_[index].BufferSize)
                        {
                            delete[] replicaBuffers_[index].Buffer;
                        }
                    }
                    delete[] replicaBuffers_;
                    replicaBuffers_ = NULL;

                    ServiceGroupReplicationEventSource::GetEvents().Error_0_OperationDataFactory(reinterpret_cast<uintptr_t>(this));
                    return hr;
                }

                WriteNoise(TraceSubComponentOperationDataFactory, "this={0} - Succeeded", this);
                count_ = bufferCount;
                return S_OK;
            }
        
        private:
            FABRIC_OPERATION_DATA_BUFFER* replicaBuffers_;
            ULONG count_;
    };
}
