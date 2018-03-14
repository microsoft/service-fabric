// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Data
{
    namespace Interop
    {
        class IsolationHelper
        {
        public: // TODO const

            enum OperationType
            {
                Invalid = 0,

                SingleEntity = 1,

                MultiEntity = 2,
            };

            static Data::TStore::StoreTransactionReadIsolationLevel::Enum GetIsolationLevel(TxnReplicator::Transaction& txn, OperationType operationType);
        };

        template <class T>
        bool IsComplete(ktl::Awaitable<T>& awaitable)
        {
            bool forceAsync = testSettings.forceAsync();
            if (forceAsync)
                return false;
            return awaitable.IsComplete();
        }
    }
}

#define CO_RETURN_VOID_ON_FAILURE(status) \
    if (!NT_SUCCESS(status)) \
    { \
        co_return; \
    } \

#define EXCEPTION_TO_STATUS(expr, status) \
    try \
    { \
        expr; \
    } \
    catch (ktl::Exception const & e) \
    { \
        status = e.GetStatus(); \
    } \

