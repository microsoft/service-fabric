// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class StoreService::ProcessPropertyBatchRequestAsyncOperation : public ProcessRequestAsyncOperation
    {
    public:
        ProcessPropertyBatchRequestAsyncOperation(
            Transport::MessageUPtr && request,
            __in NamingStore &,
            __in StoreServiceProperties &,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & root);
        
    protected:
        __declspec(property(get=get_NameWithoutFragment, put=put_NameWithoutFragment)) Common::NamingUri const & NameWithoutFragment;
        Common::NamingUri const & get_NameWithoutFragment() const { return nameWithoutFragment_; }     
        void put_NameWithoutFragment(Common::NamingUri const & value) { nameWithoutFragment_ = value; }

        DEFINE_PERF_COUNTERS ( PropertyBatch )

        Common::ErrorCode HarvestRequestMessage(Transport::MessageUPtr &&);        
        void PerformRequest(Common::AsyncOperationSPtr const &); 
        bool AllowNameFragment() override;

    private:
        void StartAcquireNamedLockForWrites(Common::AsyncOperationSPtr const &);

        void OnNamedLockAcquireComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        bool TryAcceptRequest(NamePropertyOperation const &, Common::AsyncOperationSPtr const &);

        void StartProcessingPropertyBatch(Common::AsyncOperationSPtr const &);

        void OnCommitComplete(Common::AsyncOperationSPtr const &, bool expectedCompletedSynchronously);

        void FinishProcessingPropertyBatch(Common::AsyncOperationSPtr const &, Common::ErrorCode const &);

        void DoWriteProperty(
            TransactionSPtr const &,
            NamePropertyOperation &,
            size_t operationIndex);

        void DoDeleteProperty(
            TransactionSPtr const &,
            NamePropertyOperation &,
            size_t operationIndex);

        void DoCheckPropertyExistence(
            TransactionSPtr const &,
            NamePropertyOperation &,
            size_t operationIndex);

        void DoCheckPropertySequenceNumber(
            TransactionSPtr const &,
            NamePropertyOperation &,
            size_t operationIndex); 

        void DoGetProperty(
            TransactionSPtr const &,
            NamePropertyOperation &,
            size_t operationIndex);

        void DoCheckPropertyValue(
            TransactionSPtr const &,
            NamePropertyOperation &,
            size_t operationIndex);

        void OnCompleted();

        NamePropertyOperationBatch batch_;
        NamePropertyOperationBatchResult batchResult_;
        Common::NamingUri nameWithoutFragment_;
        bool lockAcquired_;
        bool containsCheckOperation_;
        std::unique_ptr<std::unordered_map<std::wstring, _int64>> sequenceNumberCache_;
    };
}
