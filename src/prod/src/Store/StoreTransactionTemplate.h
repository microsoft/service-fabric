// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    template <class TReplicatedStore>
    class StoreTransactionTemplate : public ReplicaActivityTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(StoreTransactionTemplate);

    public:

        StoreTransactionTemplate(Store::ReplicaActivityId const &);

        StoreTransactionTemplate(StoreTransactionTemplate<TReplicatedStore> &&);

        StoreTransactionTemplate<TReplicatedStore> & operator=(StoreTransactionTemplate<TReplicatedStore> &&);

        static StoreTransactionTemplate<TReplicatedStore> CreateInvalid(
            __in TReplicatedStore *,
            __in Store::PartitionedReplicaId const &);

        static StoreTransactionTemplate<TReplicatedStore> Create(
            __in TReplicatedStore *,
            __in Store::PartitionedReplicaId const &,
            Common::ActivityId const & = Common::ActivityId());

        static StoreTransactionTemplate<TReplicatedStore> CreateSimpleTransaction(
            __in TReplicatedStore *,
            __in Store::PartitionedReplicaId const &,
            Common::ActivityId const & = Common::ActivityId());

        __declspec(property(get=get_Transaction)) IStoreBase::TransactionSPtr const & Transaction;
        __declspec(property(get=get_StoreError)) Common::ErrorCode const & StoreError;
        __declspec(property(get=get_IsReadOnly)) bool IsReadOnly;

        IStoreBase::TransactionSPtr const & get_Transaction() const { return txSPtr_; }
        Common::ErrorCode const & get_StoreError() const { return storeError_; }
        bool get_IsReadOnly() const { return isReadOnly_; }

        Common::ErrorCode Insert(StoreData const &) const;
        Common::ErrorCode Update(StoreData const &, bool checkSequenceNumber = true) const;
        Common::ErrorCode Delete(StoreData const &, bool checkSequenceNumber = true) const;

        Common::ErrorCode Exists(StoreData const &) const;
        Common::ErrorCode ReadExact(__inout StoreData &) const;

        template <class TStoreData>
        Common::ErrorCode ReadPrefix(
            std::wstring const & type,
            std::wstring const & keyPrefix,
            __out std::vector<TStoreData> &) const;

        Common::ErrorCode TryReadOrInsertIfNotFound(__inout StoreData &, __out bool & readExisting) const;
        Common::ErrorCode InsertIfNotFound(__in StoreData &, __out bool & inserted) const;
        Common::ErrorCode InsertIfNotFound(__in StoreData &) const;
        Common::ErrorCode DeleteIfFound(__in StoreData &) const;
        Common::ErrorCode UpdateOrInsertIfNotFound(__in StoreData &) const;

        void CommitReadOnly() const;
        void Rollback() const;

        static Common::AsyncOperationSPtr BeginCommit(
            StoreTransactionTemplate<TReplicatedStore> &&,
            Common::TimeSpan const, 
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        static Common::AsyncOperationSPtr BeginCommit(
            StoreTransactionTemplate<TReplicatedStore> &&,
            __in StoreData &,
            Common::TimeSpan const, 
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        static Common::AsyncOperationSPtr BeginCommit(
            StoreTransactionTemplate<TReplicatedStore> &&,
            __in StoreData &,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const &);
        static Common::ErrorCode EndCommit(Common::AsyncOperationSPtr const &);

        static Common::TimeSpan GetRandomizedOperationRetryDelay(
            Common::ErrorCode const &,
            Common::TimeSpan const maxDelay);

    private:

        // *** Member functions to be specialized

        Common::ErrorCode OnInsert(
            StoreData const &,
            __in KBuffer &) const;

        Common::ErrorCode OnUpdate(
            StoreData const &,
            ::FABRIC_SEQUENCE_NUMBER,
            __in KBuffer &) const;

        Common::ErrorCode OnDelete(
            StoreData const &,
            ::FABRIC_SEQUENCE_NUMBER) const;

        Common::ErrorCode OnCreateEnumeration(
            std::wstring const & type,
            std::wstring const & key,
            __out IStoreBase::EnumerationSPtr &) const;

        Common::ErrorCode OnReadExact(
            std::wstring const & type,
            std::wstring const & key,
            __out std::vector<byte> &,
            __out __int64 &) const;

    private:

        class CommitAsyncOperation;

        StoreTransactionTemplate(
            __in TReplicatedStore *,
            bool isValid,
            bool isSimpleTransaction,
            Store::PartitionedReplicaId const & partitionedReplicaId,
            Common::ActivityId const &);
  
        Common::ErrorCode InternalReadExact(
            std::wstring const & type,
            std::wstring const & key,
            __inout StoreData *) const;

        TReplicatedStore * store_;
        IStoreBase::TransactionSPtr txSPtr_;
        Common::ErrorCode storeError_;
        mutable bool isReadOnly_;
    };

    typedef StoreTransactionTemplate<IReplicatedStore> StoreTransaction;

    template <class TReplicatedStore> template <class TStoreData>
    Common::ErrorCode StoreTransactionTemplate<TReplicatedStore>::ReadPrefix(
        std::wstring const & type,
        std::wstring const & keyPrefix,
        __out std::vector<TStoreData> & results) const
    {
        if (!storeError_.IsSuccess()) { return storeError_; }

        std::vector<TStoreData> tempResults;
        IStoreBase::EnumerationSPtr enumSPtr;
        ErrorCode error = store_->CreateEnumerationByTypeAndKey(
            txSPtr_,
            type, 
            keyPrefix, 
            enumSPtr);

        if (error.IsSuccess())
        {
            while ((error = enumSPtr->MoveNext()).IsSuccess())
            {
                std::wstring existingType; 
                error = enumSPtr->CurrentType(existingType);

                std::wstring existingKey; 
                if (error.IsSuccess())
                {
                    error = enumSPtr->CurrentKey(existingKey);
                }

                if (error.IsSuccess())
                {
                    if (type == existingType && StringUtility::StartsWith(existingKey, keyPrefix))
                    {
                        std::vector<byte> data;
                        error = enumSPtr->CurrentValue(data);

                        if (!error.IsSuccess())
                        {
                            break;
                        }

                        TStoreData obj;
                        error = FabricSerializer::Deserialize(obj, data);
                        if (!error.IsSuccess())
                        {
                            break;
                        }

                        FABRIC_SEQUENCE_NUMBER operationLSN;
                        error = enumSPtr->CurrentOperationLSN(operationLSN);
                        if (!error.IsSuccess())
                        {
                            break;
                        }

                        obj.SetSequenceNumber(operationLSN);
                        obj.ReInitializeTracing(this->ReplicaActivityId);

                        tempResults.push_back(std::move(obj));
                    }
                    else
                    {
                        error = ErrorCodeValue::EnumerationCompleted;
                        break;
                    }
                }
            }

            if (error.IsError(ErrorCodeValue::EnumerationCompleted))
            {
                error = ErrorCodeValue::Success;
            }
        }

        if (error.IsSuccess())
        {
            results.swap(tempResults);
        }

        return error;
    }
}
