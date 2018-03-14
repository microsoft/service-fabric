// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Node
        {
            class ServiceTypeUpdatePendingLists
            {
            public:
                ServiceTypeUpdatePendingLists();

                bool TryUpdateServiceTypeLists(
                    ServiceTypeUpdateKind::Enum updateEvent,
                    ServiceModel::ServiceTypeIdentifier const& serviceTypeId);
                bool TryUpdatePartitionLists(
                    ServiceTypeUpdateKind::Enum updateEvent,
                    PartitionId const& partitionId);

                bool HasPendingUpdates() const;

                struct MessageDescription
                {
                    MessageDescription();

                    ServiceTypeUpdateKind::Enum UpdateEvent;
                    uint64 SequenceNumber;
                    vector<ServiceModel::ServiceTypeIdentifier> ServiceTypes;
                    vector<PartitionId> Partitions;
                };

                bool TryGetMessage(__out MessageDescription & description);

                typedef enum { Stale, NoPending, HasPending } ProcessReplyResult;
                ProcessReplyResult TryProcessReply(ServiceTypeUpdateKind::Enum updateEvent, uint64 sequenceNumber);

            private:
                bool CancelPendingOtherServiceTypeUpdate(
                    ServiceTypeUpdateKind::Enum updateEvent,
                    ServiceModel::ServiceTypeIdentifier const& serviceTypeId);
                bool AddPendingServiceTypeUpdate(
                    ServiceTypeUpdateKind::Enum updateEvent,
                    ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                    bool shouldIncrement);

                bool AddDisabledPartition(
                    PartitionId const& partitionId,
                    bool shouldIncrement);
                bool RemoveDisabledPartition(
                    PartitionId const& partitionId,
                    bool shouldIncrement);

                template<typename T>
                bool RemoveFromList(set<T> & list, T const& item);

                template<typename T>
                bool AddToListIfDoesNotExist(set<T> & list, T const& item);

                bool HasPendingUpdates(ServiceTypeUpdateKind::Enum updateEvent) const;

                bool TryGetNextMessageKind(__out ServiceTypeUpdateKind::Enum & kind) const;

                bool ShouldIncrementSeqNumForKind(ServiceTypeUpdateKind::Enum updateEvent) const;

                void GetPendingServiceTypes(
                    ServiceTypeUpdateKind::Enum updateEvent,
                    __out vector<ServiceModel::ServiceTypeIdentifier> & serviceTypes,
                    __out uint64 & sequenceNumber);
                void GetPendingPartitions(
                    __out vector<PartitionId> & partitions,
                    __out uint64 & sequenceNumber);

                // Message sequence number for service type disabled/enabled processing
                Common::atomic_uint64 serviceTypeUpdateMsgSeqNum_;

                // Whether or not there are disabled partitions for which a reply from FM is pending
                bool hasPendingPartitionUpdates_;

                 // Disabled ServiceTypes for which a reply from FM is pending
                std::set<ServiceModel::ServiceTypeIdentifier> pendingDisabledServices_;

                // Enabled ServiceTypes for which a reply from FM is pending
                std::set<ServiceModel::ServiceTypeIdentifier> pendingEnabledServices_;

                // Complete set of partitions that are disabled
                std::set<PartitionId> disabledPartitions_;
            };
        }
    }
}
