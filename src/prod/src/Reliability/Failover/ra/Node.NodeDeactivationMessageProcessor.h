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
            class NodeDeactivationMessageProcessor
            {
                DENY_COPY(NodeDeactivationMessageProcessor);

            public:
                NodeDeactivationMessageProcessor(
                    ReconfigurationAgent & ra);

                void ProcessActivateMessage(
                    Transport::Message & request);

                void ProcessDeactivateMessage(
                    Transport::Message & request);

                void ProcessNodeUpAck(
                    std::wstring const & activityId,
                    Reliability::FailoverManagerId const & sender,
                    NodeDeactivationInfo const & newInfo);

                void Close();

            private:
                void Process(
                    std::wstring const & activityId,
                    Reliability::FailoverManagerId const & sender,
                    NodeDeactivationInfo const & newInfo);

                void ChangeActivationState(
                    std::wstring const & activityId,
                    Reliability::FailoverManagerId const & sender,
                    NodeDeactivationInfo const & newInfo);

                void Trace(
                    std::wstring const & activityId,
                    Reliability::FailoverManagerId const & sender,
                    NodeDeactivationInfo const & newInfo) const;

                void SendReply(
                    std::wstring const & activityId,
                    Reliability::FailoverManagerId const & sender,
                    NodeDeactivationInfo const & newInfo) const;

                ReconfigurationAgent & ra_;
                NodeDeactivationStateProcessorUPtr processor_;
            };
        }
    }
}



