// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseCommitAsyncOperation : public Common::AsyncOperation
    {
    public:
        EseCommitAsyncOperation(
            EseSessionSPtr const &,
            EseLocalStoreSettings const &,
            EseStorePerformanceCounters const &,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        EseCommitAsyncOperation(
            Common::ErrorCode const &,
            EseStorePerformanceCounters const &,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        __declspec (property(get=get_CommitId)) __int64 CommitId;
        __int64 get_CommitId() const { return commitId_.commitId; }

        static Common::ErrorCode End(Common::AsyncOperationSPtr const & operation);

        void ExternalComplete(Common::AsyncOperationSPtr const &, Common::ErrorCode const &);

    protected:
        virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr);
        virtual void OnCompleted();

    private:

        void DecrementAndTryComplete(Common::AsyncOperationSPtr const &);

        EseSessionSPtr session_;
        EseStorePerformanceCounters const & perfCounters_;

        Common::Stopwatch stopwatch_;
        Common::ErrorCode error_;
        JET_COMMIT_ID commitId_;
        ULONG timeout_;
        Common::atomic_long completionCount_;

        friend class EseInstance;
    };
}
