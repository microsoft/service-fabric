// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Health
        {   
            class HealthSubsystemWrapper : public IHealthSubsystemWrapper
            {
                DENY_COPY(HealthSubsystemWrapper);
            public:
                HealthSubsystemWrapper(
                    IReliabilitySubsystemWrapperSPtr const & reliability,
                    IFederationWrapper & federation,
                    bool isHealthReportingEnabled,
                    std::wstring const nodeName);

                Common::ErrorCode ReportStoreProviderEvent(ServiceModel::HealthReport &&);

                Common::ErrorCode ReportReplicaEvent(
                    ReplicaHealthEvent::Enum type,
                    FailoverUnitId const & ftId,
                    bool isStateful,
                    uint64 replicaId,
                    uint64 instanceId,
                    HealthReportDescriptorBase const & reportDescriptor) override;

                void SendToHealthManager(
                    Transport::MessageUPtr && messagePtr,
                    Transport::IpcReceiverContextUPtr && context) override;

            private:
                IFederationWrapper & federation_;
                IReliabilitySubsystemWrapperSPtr reliability_;

                HealthContext healthContext_;
                bool const isHealthReportingEnabled_;
            };
        }
    }
}


