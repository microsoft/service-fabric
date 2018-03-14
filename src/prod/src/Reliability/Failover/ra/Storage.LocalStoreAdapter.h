// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Storage
        {
            class LocalStoreAdapter : public Storage::Api::IKeyValueStore
            {
                DENY_COPY(LocalStoreAdapter);
            public:
                LocalStoreAdapter(
                    Store::IStoreFactorySPtr const & storeFactory,
                    ReconfigurationAgent & ra);

                Store::ILocalStoreSPtr const & get_Test_LocalStore() { return store_; }

                Common::ErrorCode Open(
                    std::wstring const & nodeId, 
                    std::wstring const & workingDirectory,
                    KtlLogger::KtlLoggerNodeSPtr const & ktlLogger);

                void Close();

                Common::ErrorCode Enumerate(
                    Storage::Api::RowType::Enum type,
                    __out std::vector<Storage::Api::Row>& rows) override;

                Common::AsyncOperationSPtr BeginStoreOperation(
                    Storage::Api::OperationType::Enum operationType,
                    Storage::Api::RowIdentifier const & rowId,
                    Storage::Api::RowData && bytes,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent);

                Common::ErrorCode EndStoreOperation(Common::AsyncOperationSPtr const & operation);

            private:
                typedef Store::IStoreBase::TransactionSPtr TransactionSPtr;

                mutable Common::atomic_bool isOpen_;
                mutable Store::ILocalStoreSPtr store_;
                Store::IStoreFactorySPtr storeFactory_;
                ReconfigurationAgent & ra_;

                class CommitAsyncOperation;
                class TransactionHolder;

                Infrastructure::IThreadpool & GetThreadpool();
                Diagnostics::RAPerformanceCounters & GetPerfCounters();

                Common::ErrorCode CreateTransaction(TransactionSPtr & txPtr) const;
                Common::ErrorCode CreateTransaction(TransactionHolder & holder);

                Common::ErrorCode PerformOperationInternal(
                    Store::IStoreBase::TransactionSPtr const & txPtr,
                    Storage::Api::OperationType::Enum operationType,
                    Storage::Api::RowIdentifier const & rowId,
                    Storage::Api::RowData const & bytes);

                Common::ErrorCode TryDelete(
                    TransactionSPtr const& txPtr, 
                    std::wstring const & type, 
                    std::wstring const & id);

                Common::ErrorCode TryInsertOrUpdate(
                    TransactionSPtr const & txPtr,
                    bool isUpdate,
                    std::wstring const & type,
                    std::wstring const & id,
                    Storage::Api::RowData const & buffer);
            };
        }
    }
}
