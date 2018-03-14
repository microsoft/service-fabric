// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        //This is dummy implementation of the IReplicatedStore interface which does nothing
        //It returns success for all writes and false for all reads
        //This is used by the FMM since its store is meant to be in memory and all the caches
        //already store the data
        class FauxLocalStore : public Store::IReplicatedStore
        {
        public:
            FauxLocalStore() {}

            virtual ~FauxLocalStore() {}

        public:
            //
            // IStatefulServiceReplica
            //
            virtual Common::AsyncOperationSPtr BeginOpen(
                ::FABRIC_REPLICA_OPEN_MODE openMode,
                Common::ComPointer<::IFabricStatefulServicePartition> const & partition,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) override;

            virtual Common::ErrorCode EndOpen(
                Common::AsyncOperationSPtr const & asyncOperation, 
                __out Common::ComPointer<::IFabricReplicator> & replicator) override;

            virtual Common::AsyncOperationSPtr BeginChangeRole(
                ::FABRIC_REPLICA_ROLE newRole,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) override;

            virtual Common::ErrorCode EndChangeRole(
                Common::AsyncOperationSPtr const & asyncOperation, 
                __out std::wstring & serviceLocation) override;

            virtual Common::AsyncOperationSPtr BeginClose(
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent) override;

            virtual Common::ErrorCode EndClose(
                Common::AsyncOperationSPtr const & asyncOperation) override;

            virtual void Abort() override;

        public:
            //
            // IReplicatedStore
            //
            virtual bool GetIsActivePrimary() const override { return true; }

            virtual Common::ErrorCode GetLastCommittedSequenceNumber(__out FABRIC_SEQUENCE_NUMBER &) override { return Common::ErrorCodeValue::NotImplemented; }

            virtual ::FABRIC_TRANSACTION_ISOLATION_LEVEL GetDefaultIsolationLevel() override
            {
                return FABRIC_TRANSACTION_ISOLATION_LEVEL::FABRIC_TRANSACTION_ISOLATION_LEVEL_DEFAULT;
            }

            virtual Common::ErrorCode CreateTransaction(
                __out TransactionSPtr & transactionSPtr) override;

            virtual Common::ErrorCode CreateSimpleTransaction(
                __out TransactionSPtr & transactionSPtr) override;

            virtual Common::ErrorCode CreateEnumerationByTypeAndKey(
                __in TransactionSPtr const & transaction,
                __in std::wstring const & type,
                __in std::wstring const & keyStart,
                __out EnumerationSPtr & enumerationSPtr) override;

            virtual Common::ErrorCode Insert(
                TransactionSPtr const & transaction,
                std::wstring const & type,
                std::wstring const & key,
                __in void const * value,
                size_t valueSizeInBytes) override;

            virtual Common::ErrorCode Update(
                TransactionSPtr const & transaction,
                std::wstring const & type,
                std::wstring const & key,
                _int64 checkSequenceNumber,
                std::wstring const & newKey,
                __in_opt void const * newValue,
                size_t valueSizeInBytes) override;

            virtual Common::ErrorCode Delete(
                TransactionSPtr const & transaction,
                std::wstring const & type,
                std::wstring const & key,
                _int64 checkOperationNumber = Store::ILocalStore::SequenceNumberIgnore) override;

            virtual FILETIME GetStoreUtcFILETIME() override;

        private:

            class Transaction : public TransactionBase
            {
                DENY_COPY(Transaction);
            public:
                Transaction();
                virtual ~Transaction();

                virtual Common::AsyncOperationSPtr BeginCommit(
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & state);
                virtual Common::ErrorCode EndCommit(
                    Common::AsyncOperationSPtr const &,
                    __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber);

                virtual void AcquireLock() {}
                virtual void ReleaseLock() {}

                virtual Common::ErrorCode Commit(
                    ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber,
                    Common::TimeSpan const timeout = Common::TimeSpan::MaxValue);
                virtual void Rollback();
            };

            class Enumeration : public EnumerationBase
            {
                DENY_COPY(Enumeration);
            public:
                Enumeration();
                virtual ~Enumeration();

                virtual Common::ErrorCode MoveNext();
                virtual Common::ErrorCode CurrentOperationLSN(__inout _int64 & operationLSN);
                virtual Common::ErrorCode CurrentLastModifiedFILETIME(__inout FILETIME & fileTime);
                virtual Common::ErrorCode CurrentLastModifiedOnPrimaryFILETIME(__inout FILETIME & fileTime);
                virtual Common::ErrorCode CurrentType(__inout std::wstring & buffer);
                virtual Common::ErrorCode CurrentKey(__inout std::wstring & buffer);
                virtual Common::ErrorCode CurrentValue(__inout std::vector<byte> & buffer);
                virtual Common::ErrorCode CurrentValueSize(__inout size_t & size);
            };
        };
        typedef std::unique_ptr<FauxLocalStore> FauxLocalStoreUPtr;
    }
}
