// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

namespace ServiceGroup
{
    class CExtendedOperationDataBuffer : public Serialization::FabricSerializable
    {
    public:
        FABRIC_OPERATION_TYPE OperationType;
        FABRIC_ATOMIC_GROUP_ID AtomicGroupId;
        FABRIC_PARTITION_ID PartitionId;

        FABRIC_FIELDS_03(
            OperationType,
            AtomicGroupId,
            PartitionId
            );

        static HRESULT Read(CExtendedOperationDataBuffer & extendedOperationDataBuffer, FABRIC_OPERATION_DATA_BUFFER const & buffer);
    };

    class COutgoingOperationDataExtendedBuffer : 
        public IFabricOperationData, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(COutgoingOperationDataExtendedBuffer)
    
    public:
        //
        // Constructor/Destructor.
        //
        COutgoingOperationDataExtendedBuffer(void);
        virtual ~COutgoingOperationDataExtendedBuffer(void);

        BEGIN_COM_INTERFACE_LIST(COutgoingOperationDataExtendedBuffer)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperationData)
            COM_INTERFACE_ITEM(IID_IFabricOperationData, IFabricOperationData)
        END_COM_INTERFACE_LIST()
        //
        // IFabricOperationData methods.
        //
        STDMETHOD(GetData)(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER ** buffers);
        
        //
        // Initialization for this operation data.
        //
        STDMETHOD(FinalConstruct)(
            __in FABRIC_OPERATION_TYPE operationType, 
            __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, 
            __in IFabricOperationData* operationData,
            __in const Common::Guid& partitionId,
            __in BOOLEAN emptyOperationIgnored
            );
       //
       // Additional methods.
       //
       BOOLEAN IsEmpty(void);

       FABRIC_PARTITION_ID const & GetPartitionId();

       FABRIC_ATOMIC_GROUP_ID GetAtomicGroupId();

       FABRIC_OPERATION_TYPE GetOperationType();
       void SetOperationType(FABRIC_OPERATION_TYPE operationType);

       HRESULT SerializeExtendedOperationDataBuffer();

    protected:
        IFabricOperationData* innerOperationData_;
        CExtendedOperationDataBuffer extendedOperationData_;
        FABRIC_OPERATION_DATA_BUFFER* replicaBuffers_;
        ULONG bufferCount_;

        std::vector<BYTE> extendedOperationDataBuffer_;
    };

    class CIncomingOperationExtendedBuffer : 
        public IFabricOperation, 
        public Common::ComUnknownBase,
        public Common::TextTraceComponent<Common::TraceTaskCodes::ServiceGroupReplication>
    {
        DENY_COPY(CIncomingOperationExtendedBuffer)
    
    public:
        //
        // Constructor/Destructor.
        //
        CIncomingOperationExtendedBuffer(void);
        virtual ~CIncomingOperationExtendedBuffer(void);

        BEGIN_COM_INTERFACE_LIST(CIncomingOperationExtendedBuffer)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricOperation)
            COM_INTERFACE_ITEM(IID_IFabricOperation, IFabricOperation)
        END_COM_INTERFACE_LIST()
        //
        // IFabricOperation methods.
        //
        const FABRIC_OPERATION_METADATA * STDMETHODCALLTYPE get_Metadata(void);
        STDMETHOD(GetData)(__out ULONG* count, __out const FABRIC_OPERATION_DATA_BUFFER** buffers);
        STDMETHOD(Acknowledge)(void);
        
        //
        // Initialization/Cleanup for this operation.
        //
        STDMETHOD(FinalConstruct)(__in FABRIC_OPERATION_TYPE operationType, __in FABRIC_ATOMIC_GROUP_ID atomicGroupId, __in IFabricOperation* operation);
    
    protected:
        IFabricOperation* innerOperation_;
        FABRIC_OPERATION_METADATA operationMetadata_;
    };
}

