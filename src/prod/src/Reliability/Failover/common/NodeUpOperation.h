// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeUpOperation : public SendReceiveToFMOperation
    {
    public:
        static NodeUpOperationSPtr Create(
            IReliabilitySubsystem & reliabilitySubsystem,
            std::vector<ServicePackageInfo> && packages,
            bool isToFMM,
            bool anyReplicaFound);

    private:
        NodeUpOperation(
            IReliabilitySubsystem & reliabilitySubsystem,
            std::vector<ServicePackageInfo> && packages,
            bool isToFMM,
            bool anyReplicaFound);

        Transport::MessageUPtr CreateRequestMessage() const;

        void OnAckReceived();

        std::vector<ServicePackageInfo> packages_;
        bool anyReplicaFound_;
    };
}
