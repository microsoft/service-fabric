// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Query
        {
            class QueryHelper
            {
                DENY_COPY(QueryHelper);

            public:
                QueryHelper(
                    ReconfigurationAgent & ra);

                void ProcessMessage(
                    Transport::Message & requestMessage,
                    Federation::RequestReceiverContextUPtr && context) const;

            private:
                static ::Query::QueryMessageUtility::QueryMessageParseResult::Enum ParseQueryMessage(
                    Transport::Message & requestMessage,
                    __out ::Query::QueryRequestMessageBodyInternal & messageBodyOut,
                    __out std::wstring & activityIdOut);

                Common::ErrorCode TryGetQueryHandler(
                    ::Query::QueryRequestMessageBodyInternal & messageBody,
                    __out IQueryHandlerSPtr & handler) const;

            private:
                ReconfigurationAgent & ra_;
                // query utility
                QueryUtility queryUtility_;
                DeployedServiceReplicaUtility deployedServiceReplicaUtility_;
                // query handlers
                IQueryHandlerSPtr replicaListQueryHandler_;
                IQueryHandlerSPtr replicaDetailQueryHandler_;
            };
        }
    }
}

