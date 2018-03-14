// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            class JobItemDescription
            {
                DENY_COPY(JobItemDescription);

            public:
                JobItemDescription(
                    JobItemName::Enum name, 
                    JobItemTraceFrequency::Enum frequency,
                    bool shouldFailFastOnCommitFailure);

                __declspec(property(get = get_Name)) JobItemName::Enum Name;
                JobItemName::Enum get_Name() const { return name_; }

                __declspec(property(get = get_Frequency)) JobItemTraceFrequency::Enum Frequency;
                JobItemTraceFrequency::Enum get_Frequency() const { return frequency_; }

                __declspec(property(get = get_ShouldFailFastOnCommitFailure)) bool ShouldFailFastOnCommitFailure;
                bool get_ShouldFailFastOnCommitFailure() const { return shouldFailFastOnCommitFailure_; }

                static Common::Global<JobItemDescription> MessageProcessing;
                static Common::Global<JobItemDescription> ReconfigurationMessageRetry;
                static Common::Global<JobItemDescription> ReplicaCloseMessageRetry;
                static Common::Global<JobItemDescription> ReplicaOpenMessageRetry;
                static Common::Global<JobItemDescription> NodeUpdateService;
                static Common::Global<JobItemDescription> NodeActivationDeactivation;
                static Common::Global<JobItemDescription> Query;
                static Common::Global<JobItemDescription> ReplicaUpSender;
                static Common::Global<JobItemDescription> ReplicaUpReply;
                static Common::Global<JobItemDescription> UpdateServiceDescriptionMessageRetry;
                static Common::Global<JobItemDescription> StateCleanup;
                static Common::Global<JobItemDescription> FabricUpgradeRollback;
                static Common::Global<JobItemDescription> FabricUpgradeUpdateEntity;
                static Common::Global<JobItemDescription> ApplicationUpgradeEnumerateFTs;
                static Common::Global<JobItemDescription> ApplicationUpgradeUpdateVersionsAndCloseIfNeeded;
                static Common::Global<JobItemDescription> ApplicationUpgradeFinalizeFT;
                static Common::Global<JobItemDescription> ApplicationUpgradeReplicaDownCompletionCheck;

                static Common::Global<JobItemDescription> GetLfum;
                static Common::Global<JobItemDescription> GenerationUpdate;
                static Common::Global<JobItemDescription> NodeUpAck;

                static Common::Global<JobItemDescription> AppHostClosed;
                static Common::Global<JobItemDescription> RuntimeClosed;
                static Common::Global<JobItemDescription> ServiceTypeRegistered;

                static Common::Global<JobItemDescription> RAPQuery;
                static Common::Global<JobItemDescription> ClientReportFault;

                static Common::Global<JobItemDescription> CheckReplicaCloseProgressJobItem;
                static Common::Global<JobItemDescription> CloseFailoverUnitJobItem;

                static Common::Global<JobItemDescription> UpdateStateOnLFUMLoad;

                static Common::Global<JobItemDescription> UploadForReplicaDiscovery;

                static Common::Global<JobItemDescription> UpdateAfterFMMessageSend;
            private:
                JobItemName::Enum const name_;
                JobItemTraceFrequency::Enum const frequency_;
                bool const shouldFailFastOnCommitFailure_;
            };
        }
    }
}

