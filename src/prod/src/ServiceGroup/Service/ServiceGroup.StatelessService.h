// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricCommon.h"
#include "FabricRuntime.h"

namespace ServiceGroup
{
    //
    // Class implementing a service group made of all stateless services.
    //
    class CStatelessServiceGroup : 
        public IFabricStatelessServicePartition3,
        public IFabricStatelessServiceInstance, 
        public IFabricServiceGroupPartition,
        public CBaseServiceGroup
    {
        DENY_COPY(CStatelessServiceGroup)
    
    public:
        //
        // Constructor/Destructor.
        //
        CStatelessServiceGroup(FABRIC_INSTANCE_ID instanceId);
        virtual ~CStatelessServiceGroup(void);
    
        BEGIN_COM_INTERFACE_LIST(CStatelessServiceGroup)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition, IFabricStatelessServicePartition)
            COM_INTERFACE_ITEM(IID_IFabricStatelessServiceInstance, IFabricStatelessServiceInstance)
            COM_INTERFACE_ITEM(IID_IFabricServiceGroupPartition, IFabricServiceGroupPartition)
            COM_INTERFACE_ITEM(IID_IServiceGroupReport, IServiceGroupReport)
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
        STDMETHOD(ReportLoad)(__in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics);
        STDMETHOD(ReportFault)(__in FABRIC_FAULT_TYPE faultType);
        //
        // IFabricStatelessServicePartition1 methods
        //
        STDMETHOD(ReportMoveCost)(__in FABRIC_MOVE_COST moveCost);
        //
        // IFabricStatelessServicePartition2 methods
        //
        STDMETHOD(ReportInstanceHealth)(__in const FABRIC_HEALTH_INFORMATION* healthInformation);
        STDMETHOD(ReportPartitionHealth)(__in const FABRIC_HEALTH_INFORMATION* healthInformation);
        //
        // IFabricStatefulServicePartition3 methods.
        //
        STDMETHOD(ReportInstanceHealth2)(const FABRIC_HEALTH_INFORMATION* healthInformation, const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        STDMETHOD(ReportPartitionHealth2)(const FABRIC_HEALTH_INFORMATION* healthInformation, const FABRIC_HEALTH_REPORT_SEND_OPTIONS *sendOptions);
        //
        // IFabricServiceGroupPartition methods.
        //
        STDMETHOD(ResolveMember)(
            __in FABRIC_URI name,
            __in REFIID riid,
            __out void ** member);
        //
        // IServiceGroupReport methods.
        //
        STDMETHOD(ReportLoad)(__in FABRIC_PARTITION_ID partitionId, __in ULONG metricCount, __in const FABRIC_LOAD_METRIC* metrics);
        STDMETHOD(ReportFault)(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_FAULT_TYPE faultType);
        STDMETHOD(ReportMoveCost)(__in FABRIC_PARTITION_ID partitionId, __in FABRIC_MOVE_COST moveCost);
        //
        // Initialize/Cleanup of the service group. Overriden from base class.
        //
        STDMETHOD(FinalConstruct)(
            __in ServiceModel::CServiceGroupDescription* serviceGroupActivationInfo,
            __in const std::map<std::wstring, IFabricStatelessServiceFactory*>& statelessServiceFactories
            );
        STDMETHOD(FinalDestruct)(void);
    
    protected:
        //
        // Helper methods.
        //
        STDMETHOD(DoOpen)(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        STDMETHOD(DoClose)(
            __in IFabricAsyncOperationCallback* callback,
            __out IFabricAsyncOperationContext** context
            );
        void FixUpFailedOpen(void);
    
    private:
        //
        // Instance id of this stateless service group. System provided.
        //
        FABRIC_INSTANCE_ID instanceId_;
        //
        // Simulated partition to stateless service unit map.
        //
        std::map<Common::Guid, CStatelessServiceUnit*> innerStatelessServiceUnits_;
        //
        // Common typedefs.
        //
        typedef std::pair<Common::Guid, CStatelessServiceUnit*> PartitionId_StatelessServiceUnit_Pair;
        typedef std::map<Common::Guid, CStatelessServiceUnit*>::iterator  PartitionId_StatelessServiceUnit_Iterator;
        typedef std::pair<PartitionId_StatelessServiceUnit_Iterator, bool> PartitionId_StatelessServiceUnit_Insert;
        //
        // The stateless service partition. System provided.
        //
        IFabricStatelessServicePartition3* outerStatelessServicePartition_;
    };
}

