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
        class ComStatefulServicePartition : public IFabricStatefulServicePartition3, public IFabricInternalStatefulServicePartition, public ComServicePartitionBase, private Common::ComUnknownBase
        {
            DENY_COPY(ComStatefulServicePartition);

            BEGIN_COM_INTERFACE_LIST(ComStatefulServicePartition)
                COM_INTERFACE_ITEM(IID_IUnknown, IFabricStatefulServicePartition)
                COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition, IFabricStatefulServicePartition)
                COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition1, IFabricStatefulServicePartition1)
                COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition2, IFabricStatefulServicePartition2)
                COM_INTERFACE_ITEM(IID_IFabricStatefulServicePartition3, IFabricStatefulServicePartition3)
                COM_INTERFACE_ITEM(IID_IFabricInternalStatefulServicePartition, IFabricInternalStatefulServicePartition)
            END_COM_INTERFACE_LIST()


        public:
            ComStatefulServicePartition(
                Common::Guid const & partitionId, 
                FABRIC_REPLICA_ID replicaId, 
                ConsistencyUnitDescription const & consistencyUnitDescription, 
                bool hasPersistedState,
                std::weak_ptr<FailoverUnitProxy> && failoverUnitProxyWPtr);

            void ClosePartition();
            void AssertIsValid(Common::Guid const & partitionId, FABRIC_REPLICA_ID replicaId, FailoverUnitProxy const & owner) const;

            // IFabricStatefulServicePartition members
            HRESULT __stdcall GetPartitionInfo(FABRIC_SERVICE_PARTITION_INFORMATION const **bufferedValue);
            HRESULT __stdcall GetReadStatus(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS *readStatus);
            HRESULT __stdcall GetWriteStatus(::FABRIC_SERVICE_PARTITION_ACCESS_STATUS *writeStatus);
            HRESULT __stdcall CreateReplicator(
                __in ::IFabricStateProvider *stateProvider,
                __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
                __out ::IFabricReplicator **replicator,
                __out ::IFabricStateReplicator **stateReplicator);
            HRESULT __stdcall ReportLoad(ULONG metricCount, FABRIC_LOAD_METRIC const *metrics);
            HRESULT __stdcall ReportFault(FABRIC_FAULT_TYPE faultType);

            // IFabricStatefulServicePartition1 members
            HRESULT __stdcall ReportMoveCost(FABRIC_MOVE_COST moveCost);

            // IFabricStatefulServicePartition2 members
            HRESULT __stdcall ReportReplicaHealth(FABRIC_HEALTH_INFORMATION const *healthInformation);
            HRESULT __stdcall ReportPartitionHealth(FABRIC_HEALTH_INFORMATION const *healthInformation);

            // IFabricStatefulServicePartition3 members
            HRESULT __stdcall ReportReplicaHealth2(FABRIC_HEALTH_INFORMATION const *healthInformation, FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions);
            HRESULT __stdcall ReportPartitionHealth2(FABRIC_HEALTH_INFORMATION const *healthInformation, FABRIC_HEALTH_REPORT_SEND_OPTIONS const *sendOptions);

            // IFabricInternalStatefulServicePartition members
            HRESULT __stdcall CreateTransactionalReplicator(
                __in ::IFabricStateProvider2Factory * factory,
                __in ::IFabricDataLossHandler * dataLossHandler,
                __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
                __in_opt ::TRANSACTIONAL_REPLICATOR_SETTINGS const * transactionalReplicatorSettings,
                __in_opt KTLLOGGER_SHARED_LOG_SETTINGS const * ktlloggerSharedSettings,                                          
                __out ::IFabricPrimaryReplicator ** primaryReplicator,
                __out void ** transactionalReplicator);

            HRESULT __stdcall CreateTransactionalReplicatorInternal(
                __in ::IFabricTransactionalReplicatorRuntimeConfigurations * runtimeConfigurations,
                __in ::IFabricStateProvider2Factory * factory,
                __in ::IFabricDataLossHandler * dataLossHandler,
                __in_opt ::FABRIC_REPLICATOR_SETTINGS const *replicatorSettings,
                __in_opt ::TRANSACTIONAL_REPLICATOR_SETTINGS const * transactionalReplicatorSettings,
                __in_opt KTLLOGGER_SHARED_LOG_SETTINGS const * ktlloggerSharedSettings,                                          
                __out ::IFabricPrimaryReplicator ** primaryReplicator,
                __out void ** transactionalReplicator);

            HRESULT __stdcall GetKtlSystem(
                __out void ** ktlSystem);

            // Other members
            void SetReadWriteStatus(ReadWriteStatusValue && value);
            void OnServiceOpened();

            FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetReadStatusForQuery();
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS GetWriteStatusForQuery();

            Common::ErrorCode Test_IsLeaseExpired(bool & isLeaseExpired);

        private:

            HRESULT GetStatus(
                AccessStatusType::Enum type,
                FABRIC_SERVICE_PARTITION_ACCESS_STATUS * valueOut);

            HRESULT PrepareToCreateReplicator(__out FailoverUnitProxySPtr & failoverUnitProxy);

            Common::Guid partitionId_;
            FABRIC_REPLICA_ID replicaId_;
            ReadWriteStatusValue readWriteStatus_;
            bool isServiceOpened_;
            bool createReplicatorAlreadyCalled_;
            ComServicePartitionInfo partitionInfo_;
            bool hasPersistedState_;
        };
    }
}
