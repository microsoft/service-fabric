// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            namespace ReconfigurationAgentStoreOperation
            {
                enum Enum
                {
                    Update = 0,
                    Insert = 1,
                    Delete = 2,
                };

                void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
            }

            class ReconfigurationAgentStore
            {
                DENY_COPY(ReconfigurationAgentStore);
            public:
                ReconfigurationAgentStore(
                    Store::IStoreFactorySPtr const & storeFactory,
                    ReconfigurationAgent & ra);

                __declspec(property(get = get_LocalStore)) Store::ILocalStore & LocalStore;
                Store::ILocalStore & get_LocalStore() { return *store_; }

                Common::ErrorCode Open(
                    Federation::NodeId const & nodeId, 
                    std::wstring const & workingDirectory, 
                    KtlLogger::KtlLoggerNodeSPtr const &);

                void Close();

                template<typename T>
                Common::ErrorCode GetAllItems(std::vector<std::shared_ptr<typename EntityTraits<T>::DataType>> & items)
                {
                    typedef typename EntityTraits<T>::DataType EntityDataType;
                    typedef std::shared_ptr<EntityDataType> EntityDataSPtrType;

                    Store::IStoreBase::TransactionSPtr txPtr;

                    auto error = CreateTransaction(txPtr);
                    if (!error.IsSuccess())
                    {
                        return error;
                    }

                    Store::IStoreBase::EnumerationSPtr enumSPtr;

                    error = store_->CreateEnumerationByTypeAndKey(txPtr, EntityTraits<T>::EntityType, L"", enumSPtr);
                    if (!error.IsSuccess())
                    {
                        txPtr->Rollback();
                        return error;
                    }

                    for (;;)
                    {
                        error = enumSPtr->MoveNext();
                        if (!error.IsSuccess())
                        {
                            break;
                        }

                        EntityDataSPtrType item;
                        error = ReadCurrentData<T>(enumSPtr, item);
                        if (!error.IsSuccess())
                        {
                            break;
                        }

                        items.push_back(std::move(item));
                    }

                    if (error.IsSuccess() || error.IsError(Common::ErrorCodeValue::EnumerationCompleted))
                    {
                        error = txPtr->Commit();
                    }

                    if (!error.IsSuccess())
                    {
                        txPtr->Rollback();
                    }

                    return error;
                }

                Common::AsyncOperationSPtr BeginStoreOperation(
                    ReconfigurationAgentStoreOperation::Enum operationType,
                    std::wstring const & entityType,
                    std::wstring const & id,
                    std::vector<byte> && bytes,
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

                IThreadpool & GetThreadpool();
                Diagnostics::RAPerformanceCounters & GetPerfCounters();

                Common::ErrorCode CreateTransaction(TransactionSPtr & txPtr) const;
                Common::ErrorCode CreateTransaction(TransactionHolder & holder);

                Common::ErrorCode PerformOperationInternal(
                    Store::IStoreBase::TransactionSPtr const & txPtr,
                    ReconfigurationAgentStoreOperation::Enum operationType,
                    std::wstring const & type,
                    std::wstring const & id,
                    std::vector<byte> const & bytes);

                Common::ErrorCode TryDelete(TransactionSPtr const& txPtr, std::wstring const & type, std::wstring const & id);

                Common::ErrorCode TryInsertOrUpdate(
                    TransactionSPtr const & txPtr,
                    bool isUpdate,
                    std::wstring const & type,
                    std::wstring const & id,
                    std::vector<byte> const & buffer);

                template <class T>
                Common::ErrorCode ReadCurrentData(
                    Store::IStoreBase::EnumerationSPtr const & enumUPtr, 
                    __out std::shared_ptr<typename EntityTraits<T>::DataType> & result) const
                {
                    std::vector<byte> bytes;

                    auto error = enumUPtr->CurrentValue(bytes);
                    if (!error.IsSuccess())
                    {
                        return error;
                    }

                    result = std::make_shared<typename EntityTraits<T>::DataType>();
                    return Common::FabricSerializer::Deserialize(*result, static_cast<ULONG>(bytes.size()), bytes.data());
                }
            };
        }
    }
}
