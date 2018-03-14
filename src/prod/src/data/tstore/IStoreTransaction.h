// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        interface IStoreTransaction
        {
            K_SHARED_INTERFACE(IStoreTransaction);

        public:
            __declspec(property(get = get_ReadIsolationLevel, put = set_ReadIsolationLevel)) StoreTransactionReadIsolationLevel::Enum ReadIsolationLevel;
            virtual StoreTransactionReadIsolationLevel::Enum get_ReadIsolationLevel() const = 0;
            virtual void set_ReadIsolationLevel(__in StoreTransactionReadIsolationLevel::Enum value) = 0;

            __declspec(property(get = get_LockingHints, put = set_LockingHints)) LockingHints::Enum LockingHints;
            virtual LockingHints::Enum get_LockingHints() const = 0;
            virtual void set_LockingHints(__in LockingHints::Enum value) = 0;

            virtual void Close() = 0;
        };
    }
}
