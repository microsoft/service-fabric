// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "Store/StoreBase.h"

namespace Store
{
    class IStoreBase : public StoreBase
    {
    public:
        class TransactionBase;
        typedef std::shared_ptr<TransactionBase> TransactionSPtr;

        class EnumerationBase;
        typedef std::shared_ptr<EnumerationBase> EnumerationSPtr;

    public:

        //
        // Default isolation level for the ILocalStore instance
        //
        virtual ::FABRIC_TRANSACTION_ISOLATION_LEVEL GetDefaultIsolationLevel() = 0;

        //
        // Called to set/change the default isolation level of the ILocalStore instance
        //
        virtual Common::ErrorCode SetDefaultIsolationLevel(::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel)
        {
            return GetDefaultIsolationLevel() == isolationLevel ? Common::ErrorCodeValue::Success : Common::ErrorCodeValue::InvalidOperation;
        }

        //
        // Create a transaction with the specific isolation level
        //
        virtual Common::ErrorCode CreateTransaction(
            __in ::FABRIC_TRANSACTION_ISOLATION_LEVEL isolationLevel,
            __out TransactionSPtr & transactionSPtr)
        {
            return GetDefaultIsolationLevel() == isolationLevel ? CreateTransaction(transactionSPtr) : Common::ErrorCodeValue::InvalidOperation;
        }

        //
        // Create a transaction with the default isolation level of the ILocalStore instance
        //
        virtual Common::ErrorCode CreateTransaction(
            __out TransactionSPtr & transactionSPtr) = 0;

        virtual FILETIME GetStoreUtcFILETIME() = 0;

        class TransactionBase
        {
        public:
            virtual void Abort() { }

            virtual Common::ErrorCode CheckAborted() { return Common::ErrorCodeValue::Success; }

            virtual Common::AsyncOperationSPtr BeginCommit(
                __in Common::TimeSpan const timeout,
                __in Common::AsyncCallback const & callback,
                __in Common::AsyncOperationSPtr const & state)
            {
                auto error = Commit(timeout);
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(error, callback, state);
            }

            virtual Common::ErrorCode EndCommit(
                __in Common::AsyncOperationSPtr const & operation,
                __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber)
            {
                commitSequenceNumber = -1;
                return Common::AsyncOperation::End<Common::CompletedAsyncOperation>(operation)->Error;
            }

            virtual Common::ErrorCode EndCommit(
                __in Common::AsyncOperationSPtr const & operation)
            {
                ::FABRIC_SEQUENCE_NUMBER dontCareSequenceNumber;
                return EndCommit(operation, dontCareSequenceNumber);
            }

            virtual Common::ErrorCode Commit(
                __in Common::TimeSpan const timeout = Common::TimeSpan::MaxValue)
            {
                ::FABRIC_SEQUENCE_NUMBER dontCareSequenceNumber;
                return Commit(dontCareSequenceNumber, timeout);
            }

            virtual Common::ErrorCode Commit(
                __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber,
                __in Common::TimeSpan const timeout = Common::TimeSpan::MaxValue) = 0;

            virtual Common::AsyncOperationSPtr BeginRollback(
                __in Common::AsyncCallback const & callback,
                __in Common::AsyncOperationSPtr const & state)
            {
                Rollback();
                return Common::AsyncOperation::CreateAndStart<Common::CompletedAsyncOperation>(Common::ErrorCodeValue::Success, callback, state);
            }

            virtual void EndRollback(
                __in Common::AsyncOperationSPtr const & operation)
            {
                Common::AsyncOperation::End<Common::CompletedAsyncOperation>(operation);
            }

            virtual void Rollback() = 0;
        };

        // Creates a context for enumerating all items in the store of a given type, and
        // optionally starting at a given key.  The type must match exactly, and the order of
        // items must be in sorted key order.
        virtual Common::ErrorCode CreateEnumerationByTypeAndKey(
            __in TransactionSPtr const & transaction,
            __in std::wstring const & type,
            __in std::wstring const & keyStart,
            __out EnumerationSPtr & enumerationSPtr) = 0;

        // V2 stack enumeration creation has an initialization cost that's
        // linear wrt the amount of data in the store (it will linearly scan to
        // the first key match). Expose an exact read interface to optimize
        // for V2 stack reads and avoid enumeration where possible.
        //
        virtual Common::ErrorCode ReadExact(
            __in TransactionSPtr const & tx,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __out std::vector<byte> & value,
            __out __int64 & operationLsn)
        {
            FILETIME unusedLastModified;
            return ReadExact(tx, type, key, value, operationLsn, unusedLastModified);
        }

        virtual Common::ErrorCode ReadExact(
            __in TransactionSPtr const & tx,
            __in std::wstring const & type,
            __in std::wstring const & key,
            __out std::vector<byte> & value,
            __out __int64 & operationLsn,
            __out __out FILETIME & lastModified)
        {
            EnumerationSPtr enumSPtr;
            auto error = this->CreateEnumerationByTypeAndKey(tx, type, key, enumSPtr);
            if (!error.IsSuccess()) { return error; }

            error = enumSPtr->MoveNext();
            if (error.IsError(Common::ErrorCodeValue::EnumerationCompleted)) { return Common::ErrorCodeValue::NotFound; }
            if (!error.IsSuccess()) { return error; }

            std::wstring currentType;
            error = enumSPtr->CurrentType(currentType);
            if (!error.IsSuccess()) { return error; }
            
            std::wstring currentKey;
            error = enumSPtr->CurrentKey(currentKey);
            if (!error.IsSuccess()) { return error; }

            if (currentType == type && currentKey == key)
            {
                error = enumSPtr->CurrentValue(value);
                if (!error.IsSuccess()) { return error; }

                error = enumSPtr->CurrentOperationLSN(operationLsn);
                if (!error.IsSuccess()) { return error; }

                error = enumSPtr->CurrentLastModifiedFILETIME(lastModified);
                if (!error.IsSuccess()) { return error; }
            }
            else
            {
                error = Common::ErrorCodeValue::NotFound;
            }

            return error;
        }

        class EnumerationBase
        {
        public:
            virtual ~EnumerationBase() { }
            virtual Common::ErrorCode MoveNext() = 0;
            virtual Common::ErrorCode CurrentOperationLSN(__inout _int64 & operationNumber) = 0;
            virtual Common::ErrorCode CurrentLastModifiedFILETIME(__inout FILETIME & fileTime) = 0;
            virtual Common::ErrorCode CurrentLastModifiedOnPrimaryFILETIME(__inout FILETIME & fileTime) = 0;

            // Clears current contents and resizes to the size of the value.
            virtual Common::ErrorCode CurrentType(__inout std::wstring & buffer) = 0;
            virtual Common::ErrorCode CurrentKey(__inout std::wstring & buffer) = 0;
            virtual Common::ErrorCode CurrentValue(__inout std::vector<byte> & buffer) = 0;
            virtual Common::ErrorCode CurrentValueSize(__inout size_t & size) = 0;
        };

    protected:
        template <class TDerived>
        TDerived & CastTransaction(
            __in IStoreBase::TransactionSPtr const & transaction)
        {
            return *(static_cast<TDerived *>(transaction.get()));
        }

        template <class TDerived>
        std::shared_ptr<TDerived> CastTransactionSPtr(
            __in IStoreBase::TransactionSPtr const & transaction)
        {
            return std::dynamic_pointer_cast<TDerived>(transaction);
        }
    };
}
