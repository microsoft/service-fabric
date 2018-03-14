// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReliabilitySubsystemWrapper : public ReconfigurationAgentComponent::IReliabilitySubsystemWrapper
    {
        DENY_COPY(ReliabilitySubsystemWrapper)
    public:
        ReliabilitySubsystemWrapper(ReliabilitySubsystem * reliability);

        std::wstring const & get_WorkingDirectory() const;
        Common::FabricNodeConfigSPtr const & get_NodeConfig();

        void SendNodeUpToFM();

        void SendNodeUpToFMM();

        void UploadLFUM(
            Reliability::GenerationNumber const & generation,
            std::vector<Reliability::FailoverUnitInfo> && failoverUnitInfo,
            bool anyReplicaFound);

        void UploadLFUMForFMReplicaSet(
            Reliability::GenerationNumber const & generation,
            std::vector<Reliability::FailoverUnitInfo> && failoverUnitInfo,
            bool anyReplicaFound);

        void ForwardHealthReportFromRAPToHealthManager(
            Transport::MessageUPtr && messagePtr,
            Transport::IpcReceiverContextUPtr && ipcTransportContext);

        Common::ErrorCode AddHealthReport(ServiceModel::HealthReport && healthReport);

    private:
        void SendToHealthManagerCompletedCallback(Common::AsyncOperationSPtr const & asyncOperation, Transport::IpcReceiverContextUPtr && context);
        ReliabilitySubsystem * reliability_;
    };
}
