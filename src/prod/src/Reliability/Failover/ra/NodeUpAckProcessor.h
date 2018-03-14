// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        class NodeUpAckProcessor
        {
            DENY_COPY(NodeUpAckProcessor);

        public:
            NodeUpAckProcessor(ReconfigurationAgent & ra);

            void ProcessNodeUpAck(
                std::wstring && activityId,
                NodeUpAckMessageBody && body,
                FailoverManagerId const & fmId);

            bool NodeUpAckFTProcessor(
                Infrastructure::HandlerParameters & handlerParameters, 
                JobItemContextBase & context);

        private:

            bool TryUpgradeFabric(
                Reliability::FailoverManagerId const & sender,
                std::wstring const & activityId,
                NodeUpAckMessageBody const & nodeUpAckBody);

            void ProcessNodeUpAck(
                Reliability::FailoverManagerId const & sender,
                std::wstring && activityId,
                NodeUpAckMessageBody && body);

            class NodeUpAckProcessResult
            {
            public:
                enum Enum
                {
                    Success = 0,
                    NodeUpgrading = 1,
                    ResendNodeUp = 2,
                };
            };

            ReconfigurationAgent & ra_;
        };
    }
}

