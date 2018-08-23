// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class HealthSubsystemWrapperStub : public Health::IHealthSubsystemWrapper
            {
            public:
                struct ReplicaHealthEventInfo
                {
                    ReplicaHealthEventInfo(Health::HealthReportDescriptorBase const && desc) : Descriptor(desc)
                    {
                    }

                    Reliability::FailoverUnitId FTId;
                    uint64 ReplicaId;
                    uint64 InstanceId;
                    Health::ReplicaHealthEvent::Enum Type;
                    bool IsStateful;
                    Health::HealthReportDescriptorBase const & Descriptor;
                };

                __declspec(property(get = get_ReplicaHealthEvents)) std::vector<ReplicaHealthEventInfo> & ReplicaHealthEvents;
                std::vector<ReplicaHealthEventInfo> & get_ReplicaHealthEvents()  { return replicaHealthEvents_; }

                Common::ErrorCode ReportStoreProviderEvent(ServiceModel::HealthReport &&)
                {
                    return Common::ErrorCodeValue::Success;
                }

                Common::ErrorCode ReportReplicaEvent(
                    Health::ReplicaHealthEvent::Enum type,
                    Reliability::FailoverUnitId const & ftId,
                    bool isStateful,
                    uint64 replicaId,
                    uint64 instanceId,
                    Health::HealthReportDescriptorBase const & descriptor) override
                {
                    ReplicaHealthEventInfo info (move(descriptor));
                    info.Type = type;
                    info.FTId = ftId;
                    info.ReplicaId = replicaId;
                    info.InstanceId = instanceId;
                    info.IsStateful = isStateful;

                    replicaHealthEvents_.push_back(info);

                    return Common::ErrorCodeValue::Success;
                }

                void SendToHealthManager(
                    Transport::MessageUPtr && messagePtr,
                    Transport::IpcReceiverContextUPtr && context) override
                {
                    UNREFERENCED_PARAMETER(messagePtr);
                    UNREFERENCED_PARAMETER(context);
                }

            private:
                std::vector<ReplicaHealthEventInfo> replicaHealthEvents_;
            };

            class PerfTestHealthSubsystemWrapperStub : public Health::IHealthSubsystemWrapper
            {
            public:
                Common::ErrorCode ReportStoreProviderEvent(ServiceModel::HealthReport &&)
                {
                    return Common::ErrorCodeValue::Success;
                }

                Common::ErrorCode ReportReplicaEvent(
                    Health::ReplicaHealthEvent::Enum ,
                    Reliability::FailoverUnitId const & ,
                    bool ,
                    uint64 ,
                    uint64 ,
                    Health::HealthReportDescriptorBase const &) override
                {
                    return Common::ErrorCodeValue::Success;
                }

                void SendToHealthManager(
                    Transport::MessageUPtr && messagePtr,
                    Transport::IpcReceiverContextUPtr && context) override
                {
                    UNREFERENCED_PARAMETER(messagePtr);
                    UNREFERENCED_PARAMETER(context);
                }
            };
        }
    }
}

