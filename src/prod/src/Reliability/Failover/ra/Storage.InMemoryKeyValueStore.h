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
            class InMemoryKeyValueStore : public Api::IKeyValueStore
            {
            public:
                InMemoryKeyValueStore() :
                    state_(make_shared<InMemoryKeyValueStoreState>())
                {
                }

                InMemoryKeyValueStore(InMemoryKeyValueStoreStateSPtr const & state) :
                    state_(state)
                {
                    ASSERT_IF(state == nullptr, "state cant be null");
                }

                Common::ErrorCode Enumerate(
                    Storage::Api::RowType::Enum type,
                    __out std::vector<Storage::Api::Row>&  rows) override
                {
                    Common::AcquireExclusiveLock grab(lock_);
                    return state_->Enumerate(type, rows);
                }

                Common::AsyncOperationSPtr BeginStoreOperation(
                    Storage::Api::OperationType::Enum operationType,
                    Storage::Api::RowIdentifier const & rowId,
                    Storage::Api::RowData && bytes,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    UNREFERENCED_PARAMETER(timeout);

                    Common::ErrorCode error;

                    {
                        Common::AcquireExclusiveLock grab(lock_);
                        error = state_->PerformOperation(operationType, rowId, move(bytes));
                    }

                    return Common::AsyncOperation::CreateAndStart<CompletedAsyncOperation>(error, callback, parent);
                }

                Common::ErrorCode EndStoreOperation(Common::AsyncOperationSPtr const & operation) override
                {
                    return Common::AsyncOperation::End(operation);
                }

                void Close() override
                {
                }

            private:
                Common::ExclusiveLock lock_;
                InMemoryKeyValueStoreStateSPtr state_;
            };
        }
    }
}


