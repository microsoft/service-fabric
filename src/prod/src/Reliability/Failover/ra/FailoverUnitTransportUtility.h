// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class FailoverUnitTransportUtility
        {
        public:
            FailoverUnitTransportUtility(Reliability::FailoverManagerId const & owner) : owner_(owner)
            {
            }

            template <typename T>
            void AddSendMessageToFMAction(
                Infrastructure::StateMachineActionQueue & actionQueue,
                RSMessage const & messageType,
                T && body) const
            {
                actionQueue.Enqueue(
                    make_unique<Infrastructure::SendMessageToFMAction<T>>(
                        owner_,
                        messageType,
                        move(body)));
            }

            void AddRequestFMMessageRetryAction(
                Infrastructure::StateMachineActionQueue & actionQueue)
            {
                actionQueue.Enqueue(make_unique<Communication::RequestFMMessageRetryAction>(owner_));
            }

        private:
            Reliability::FailoverManagerId owner_;
        };
    }
}



