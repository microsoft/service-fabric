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
            class IHealthSubsystemWrapper
            {
            public:
                virtual ~IHealthSubsystemWrapper() {}

                virtual Common::ErrorCode ReportStoreProviderEvent(ServiceModel::HealthReport &&) = 0;

                virtual Common::ErrorCode ReportReplicaEvent(
                    ReplicaHealthEvent::Enum type,
                    FailoverUnitId const & ftId,
                    bool isStateful,
                    uint64 replicaId,
                    uint64 instanceId,
                    HealthReportDescriptorBase const & reportDescriptor) = 0;

                virtual void SendToHealthManager(
                    Transport::MessageUPtr && messagePtr,
                    Transport::IpcReceiverContextUPtr && context) = 0;

                static void EnqueueReplicaHealthAction(
                    ReplicaHealthEvent::Enum type,
                    FailoverUnitId const & ftId,
                    bool isStateful,
                    uint64 replicaId,
                    uint64 instanceId,
                    Infrastructure::StateMachineActionQueue & queue,
                    IHealthDescriptorSPtr const & reportDescriptor);
            };

        }
    }
}
