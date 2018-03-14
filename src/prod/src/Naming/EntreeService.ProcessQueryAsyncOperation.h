// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService::ProcessQueryAsyncOperation : public Common::TimedAsyncOperation
    {
    public:
        ProcessQueryAsyncOperation(
            __in EntreeService & owner,
            Query::QueryNames::Enum queryName,
            ServiceModel::QueryArgumentMap const & queryArgs,
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode End(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out Transport::MessageUPtr &reply);

    protected:
         void OnStart(Common::AsyncOperationSPtr const & thisSPtr);

    private:
        void HandleGetQueryListAsync(Common::AsyncOperationSPtr const & thisSPtr);
        void OnGetQueryListCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        void HandleGetApplicationNameAsync(Common::AsyncOperationSPtr const & thisSPtr);
        void OnGetApplicationNameCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

        EntreeService & owner_;
        Query::QueryNames::Enum queryName_;
        ServiceModel::QueryArgumentMap queryArgs_;
        Common::ActivityId const &activityId_;
        Transport::MessageUPtr reply_;
    };
}
