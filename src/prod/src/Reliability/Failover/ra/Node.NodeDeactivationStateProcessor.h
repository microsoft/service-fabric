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
            /*
                Handles processing of changes to the node activation state
                Performs the work of closing and opening the replicas
            */
            class NodeDeactivationStateProcessor
            {
                DENY_COPY(NodeDeactivationStateProcessor);

            public:
                NodeDeactivationStateProcessor(
                    ReconfigurationAgent & ra) :
                    ra_(ra)
                {
                }

                void ProcessActivationStateChange(
                    std::wstring const & activityId,
                    NodeDeactivationState & state_,
                    NodeDeactivationInfo const & newInfo);

            private:
                void OnFailoverUnitUpdateComplete(
                    Infrastructure::MultipleEntityWork & work, 
                    NodeDeactivationState & state,
                    NodeDeactivationInfo const & info);

                bool UpdateFailoverUnit(
                    Infrastructure::HandlerParameters & handlerParameters,
                    NodeDeactivationState & state,
                    NodeDeactivationInfo const & info);

                ReconfigurationAgent & ra_;
            };
        }
    }
}



