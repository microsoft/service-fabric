// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Testability
{
    class UnreliableLeaseHelper;
    typedef std::unique_ptr<UnreliableLeaseHelper> UnreliableLeaseHelperUPtr;
    typedef std::shared_ptr<UnreliableLeaseHelper> UnreliableLeaseHelperSPtr;

    class UnreliableLeaseHelper
    {
        DENY_COPY(UnreliableLeaseHelper)

    public:
        UnreliableLeaseHelper();

        ~UnreliableLeaseHelper();
        Common::ErrorCode AddUnreliableLeaseBehavior(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);

        Common::ErrorCode RemoveUnreliableLeaseBehavior(
            ServiceModel::QueryArgumentMap const & queryArgument,
            Common::ActivityId const & activityId,
            __out ServiceModel::QueryResult & queryResult);
      
    };
}
