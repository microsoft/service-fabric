// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{    
    class StoreService::ProcessEnumerateSubNamesRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessEnumerateSubNamesRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:

        DEFINE_PERF_COUNTERS ( EnumerateNames )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);
        void PerformRequest(Common::AsyncOperationSPtr const &);

    private:
        Common::ErrorCode InitializeEnumeration(TransactionSPtr const &, __out EnumerationSPtr &);

        Common::ErrorCode EnumerateSubnames(EnumerationSPtr &&);

        void StartResolveTentativeSubnames(Common::AsyncOperationSPtr const &);

        void OnNamedLockAcquiredForParentName(Common::AsyncOperationSPtr const & lockOperation, bool expectedCompletedSynchronously);
        void OnNamedLockAcquiredForTentativeSubname(
            Common::NamingUri const &, 
            HierarchyNameData const &, 
            Common::AsyncOperationSPtr const & lockOperation, 
            bool expectedCompletedSynchronously);

        Common::ErrorCode ReadHierarchyNameData(TransactionSPtr const &, Common::NamingUri const &, __out HierarchyNameData &);

        void SetConsistentFlag(bool);
        void SetFinishedFlag(bool);

        bool FitsInReplyMessage(Common::NamingUri const &);
        void SetReplyMessage();

        EnumerateSubNamesRequest body_;
        std::vector<std::wstring> allSubnames_;
        std::map<Common::NamingUri, HierarchyNameData> tentativeSubnames_;
        Common::atomic_long pendingNameReads_;
        std::map<Common::NamingUri, HierarchyNameData> failedTentativeSubnames_;
        Common::atomic_bool hasFailedPendingNameReads_;
        Common::ExclusiveLock tentativeSubnamesLock_;

        Common::NamingUri lastEnumeratedName_;
        FABRIC_ENUMERATION_STATUS enumerationStatus_;
        _int64 subnamesVersion_;

        size_t replySizeThreshold_;
        size_t replyItemsSizeEstimate_;
        size_t replyTotalSizeEstimate_;
    };
}
