// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{   
    class ILegacyTestabilityRequestHandler
    {
        DENY_COPY(ILegacyTestabilityRequestHandler);

    public:
        ILegacyTestabilityRequestHandler() = default;
        virtual ~ILegacyTestabilityRequestHandler() = default;

        virtual Common::AsyncOperationSPtr BeginTestabilityOperation(
            Query::QueryNames::Enum queryName,
            ServiceModel::QueryArgumentMap const & queryArgs,
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndTestabilityOperation(
            Common::AsyncOperationSPtr const &operation,
            __out Transport::MessageUPtr & reply) = 0;
    };
}
