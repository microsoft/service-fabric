// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ChangeNotificationOperation : public SendReceiveToFMOperation
    {
    public:
        static ChangeNotificationOperationSPtr Create(IReliabilitySubsystem & reliabilitySubsystem, bool isToFMM);

    private:
        ChangeNotificationOperation(IReliabilitySubsystem & reliabilitySubsystem, bool isToFMM);

        Transport::MessageUPtr CreateRequestMessage() const;

        Common::TimeSpan GetDelay() const;

        bool IsChallenged() const;
    };
}
