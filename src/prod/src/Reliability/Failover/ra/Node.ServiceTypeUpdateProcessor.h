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
            class ServiceTypeUpdateProcessor
            {
                DENY_COPY(ServiceTypeUpdateProcessor);

            public:
                ServiceTypeUpdateProcessor(
                    ReconfigurationAgent & ra,
                    TimeSpanConfigEntry const& cleanupInterval,
                    TimeSpanConfigEntry const& stalenessEntryTtl);

                __declspec(property(get = get_MessageRetryWorkManager)) Infrastructure::BackgroundWorkManagerWithRetry& MessageRetryWorkManager;
                Infrastructure::BackgroundWorkManagerWithRetry& get_MessageRetryWorkManager() { return *messageWorkManager_; }

                void ProcessServiceTypeDisabled(
                    ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                    uint64 sequenceNumber,
                    ServiceModel::ServicePackageActivationContext const& activationContext);
                void ProcessServiceTypeEnabled(
                    ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                    uint64 sequenceNumber,
                    ServiceModel::ServicePackageActivationContext const& activationContext);

                void ServiceTypeEnabledReplyHandler(Transport::Message & request);
                void ServiceTypeDisabledReplyHandler(Transport::Message & request);
                void PartitionNotificationReplyHandler(Transport::Message & request);

                void Close();

            private:
                void Open();
                void CreateMessageWorkManager();

                void ServiceTypeUpdateProcessor::ProcessServiceTypeUpdate(
                    ServiceTypeUpdateKind::Enum updateEvent,
                    ServiceModel::ServiceTypeIdentifier const& serviceTypeId,
                    uint64 sequenceNumber);
                void ProcessPartitionUpdate(
                    ServiceTypeUpdateKind::Enum updateEvent,
                    PartitionId partitionId,
                    uint64 sequenceNumber);

                bool MessageSendCallback();

                void SendServiceTypeUpdateMessage(ServiceTypeUpdatePendingLists::MessageDescription && description);
                void SendPartitionNotificationMessage(ServiceTypeUpdatePendingLists::MessageDescription && description);

                void ServiceTypeUpdateReplyHandler(ServiceTypeUpdateKind::Enum updateEvent, Transport::Message& request);

                void CancelRetryTimer(int64 sequenceNumber);

                void OnCleanupTimerCallback();

                ReconfigurationAgent & ra_;
                Infrastructure::BackgroundWorkManagerWithRetryUPtr messageWorkManager_;

                // Lock for synchronizing access to state
                mutable Common::RwLock lock_;

                ServiceTypeUpdateStalenessChecker stalenessChecker_;
                Infrastructure::RetryTimer stalenessCleanupTimer_;

                ServiceTypeUpdatePendingLists pendingLists_;
            };
        }
    }
}
