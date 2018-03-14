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
            class IQueryHandler 
            {
            DENY_COPY(IQueryHandler);

            public:
                virtual void ProcessQuery(
                    Federation::RequestReceiverContextUPtr && context,
                    ServiceModel::QueryArgumentMap const & queryArgs,
                    std::wstring const & activityId) = 0;

                IQueryHandler() {}

                virtual ~IQueryHandler()
                {
                }
            };
        }
    }
}
