// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

#include "ServiceGroup.AsyncOperations.h"

namespace ServiceGroup
{
    //
    // Stateless service group unit hosting a user stateless service instance.
    //
    class CStatelessServiceUnit : 
        public IFabricStatelessServicePartition3,
        public IFabricStatelessServiceInstance, 
        public IFabricServiceGroupPartition,
        public CBaseServiceUnit
    {
    
        DENY_COPY(CStatelessServiceUnit)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessServiceUnit(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_INSTANCE_ID instanceId);
        virtual ~CStatelessServiceUnit(void);
    
        BEGIN_COM_INTERFACE_LIST(CStatelessServiceUnit)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition, IFabricStatelessServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupPartition, IFabricServiceGroupPartition)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition1, IFabricStatelessServicePartition1)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition2, IFabricStatelessServicePartition2)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition3, IFabricStatelessServicePartition3)
        END_COM_INTERFACE_LIST()
        //
        // IFabricStatelessServiceInstance methods.
        //
        STDMETHOD(BeginOpen)(
            __in IFabricStatelessServicePartition* partition,
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndOpen)(
            __in IFabricAsyncOperationContext* context,
            __out IFabricStringResult** serviceEndpoint
            );
        STDMETHOD(BeginClose)(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(EndClose)(__in IFabricAsyncOperationContext* context);
        STDMETHOD_(void, Abort)();
        //
        // IFabricStatelessServicePartition methods.
        //
        STDMETHOD(GetPartitionInfo)(__out const FABRIC_SERVICE_PARTITION_INFORMATION** bufferedValue);
        STDMETHOD(ReportLoad)(
            __in ULONG metricCount,
            __in const FABRIC_LOAD_METRIC* metrics
            );
        STDMETHOD(ReportFault)(
            __in FABRIC_FAULT_TYPE faultType
            );
        //
        // IFabricStatelessServicePartition1 methods.
        //
        STDMETHOD(ReportMoveCost)(
            __in FABRIC_MOVE_COST moveCost
            );
        //
        // IFabricStatelessServicePartition2 methods
        //
        STDMETHOD(ReportInstanceHealth)(__in const FABRIC_HEALTH_INFORMATION* healthInformation);
        STDMETHOD(ReportPartitionHealth)(__in const FABRIC_HEALTH_INFORMATION* healthInformation);
        //
        // IFabricStatefulServicePartition3 methods.
        //
        STDMETHOD(ReportInstanceHealth2)(__in const FABRIC_HEALTH_INFORMATION* healthInformation, const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        STDMETHOD(ReportPartitionHealth2)(__in const FABRIC_HEALTH_INFORMATION* healthInformation, const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        //
        // IFabricServiceGroupPartition methods.
        //
        STDMETHOD(ResolveMember)(
            __in FABRIC_URI name,
            __in REFIID riid,
            __out void ** member);
        //
        // Initializes the stateless service unit. Overrides from base class.
        //
        STDMETHOD(FinalConstruct)(
            __in ServiceModel::CServiceGroupMemberDescription* serviceActivationInfo,
            __in IFabricStatelessServiceFactory* factory
            );
        STDMETHOD(FinalDestruct)(void);
        //
        // Inner stateless service instance query interface.
        //
        HRESULT InnerQueryInterface(__in REFIID riid, __out void ** ppvObject);
        //
        // Used to release resources acquired during open.
        //
        void FixUpFailedOpen(void);
        
    private:
        //
        // Outer stateless service partition. System provided.
        //
        IFabricStatelessServicePartition3* outerStatelessServicePartition_;
        //
        // Outer service group partition.
        //
        IFabricServiceGroupPartition* outerServiceGroupPartition_;
        //
        // Replica id of this stateful service unit. System provided.
        //
        FABRIC_INSTANCE_ID instanceId_;
        //
        // Service instance contained by the service group. User service provided.
        //
        IFabricStatelessServiceInstance* innerStatelessServiceInstance_;
    };
}
