// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class LFUMUploadOperation : public Reliability::SendReceiveToFMOperation
        {
        public:
            LFUMUploadOperation(
                Reliability::IReliabilitySubsystem & reliabilitySubsystem,
                Reliability::GenerationNumber const & generation,
                Reliability::NodeDescription && node,
                std::vector<FailoverUnitInfo> && failoverUnitInfos,
                bool isToFMM,
                bool anyReplicaFound);

        private:
            Transport::MessageUPtr CreateRequestMessage() const;

            void OnAckReceived();

            Reliability::LFUMMessageBody body_;
        };
    }
}
