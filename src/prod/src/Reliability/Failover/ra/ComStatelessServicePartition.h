// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        // Represents the service partition object that is given to the service
        class ComStatelessServicePartition : public IFabricStatelessServicePartition3, public ComServicePartitionBase, private Common::ComUnknownBase
        {
            DENY_COPY(ComStatelessServicePartition);

            BEGIN_COM_INTERFACE_LIST(ComStatelessServicePartition)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatelessServicePartition)
                COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition, IFabricStatelessServicePartition)
                COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition1, IFabricStatelessServicePartition1)
                COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition2, IFabricStatelessServicePartition2)
                COM_INTERFACE_ITEM(IID_IFabricStatelessServicePartition3, IFabricStatelessServicePartition3)
            END_COM_INTERFACE_LIST()

        public:
            ComStatelessServicePartition(
                Common::Guid const & partitionId,
                ConsistencyUnitDescription const & consistencyUnitDescription,
                FABRIC_REPLICA_ID replicaId,
                std::weak_ptr<FailoverUnitProxy> && failoverUnitProxyWPtr);
            
            void ClosePartition();
            void AssertIsValid(Common::Guid const & partitionId, FABRIC_REPLICA_ID replicaId, FailoverUnitProxy const & owner) const;

            // IFabricStatelessServicePartition members
            HRESULT __stdcall GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const **bufferedValue);
            HRESULT __stdcall ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics);
            HRESULT __stdcall ReportFault(FABRIC_FAULT_TYPE faultType);

            // IFabricStatelessServicePartition1 members
            HRESULT __stdcall ReportMoveCost(FABRIC_MOVE_COST moveCost);

            // IFabricStatelessServicePartition2 members
            HRESULT __stdcall ReportInstanceHealth(FABRIC_HEALTH_INFORMATION const *healthInformation);
            HRESULT __stdcall ReportPartitionHealth(FABRIC_HEALTH_INFORMATION const *healthInformation);

            // IFabricStatelessServicePartition3 members
            HRESULT __stdcall ReportInstanceHealth2(FABRIC_HEALTH_INFORMATION const *healthInformation, FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions);
            HRESULT __stdcall ReportPartitionHealth2(FABRIC_HEALTH_INFORMATION const *healthInformation, FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions);

        private:       
            ComServicePartitionInfo partitionInfo_;
            Common::Guid partitionId_;
            FABRIC_REPLICA_ID replicaId_;

        };
    }
}
