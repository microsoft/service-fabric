// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace MessageRetry
        {
            // Class responsible for taking an enumeration result and sending the messages
            class FMMessageSender
            {
                DENY_COPY(FMMessageSender);

            public:
                FMMessageSender(
                    Node::PendingReplicaUploadStateProcessor const & pendingReplicaUploadState,
                    Communication::FMTransport & transport,
                    Reliability::FailoverManagerId const & target);

                void Send(
                    std::wstring const & activityId,
                    __inout std::vector<Communication::FMMessageDescription> & messages,
                    __inout Diagnostics::FMMessageRetryPerformanceData & perfData);

            private:
                Node::PendingReplicaUploadStateProcessor const & pendingReplicaUploadState_;
                Communication::FMTransport & transport_;
                Reliability::FailoverManagerId target_;
            };
        }
    }
}



