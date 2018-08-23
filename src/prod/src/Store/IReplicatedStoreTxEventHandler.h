//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Store
{
    class IReplicatedStoreTxEventHandler
    {
    public:
        typedef uint64 TransactionMapKey;

        virtual void OnCreateTransaction(Common::ActivityId const &, TransactionMapKey) = 0;
        virtual void OnReleaseTransaction(Common::ActivityId const &, TransactionMapKey) = 0;

        virtual Common::ErrorCode OnInsert(
            Common::ActivityId const &,
            TransactionMapKey,
            std::wstring const & type,
            std::wstring const & key,
            void const * value,
            size_t const valueSizeInBytes) = 0;

        virtual Common::ErrorCode OnUpdate(
            Common::ActivityId const &,
            TransactionMapKey,
            std::wstring const & type,
            std::wstring const & key,
            void const * newValue,
            size_t const valueSizeInBytes) = 0;

        virtual Common::ErrorCode OnDelete(
            Common::ActivityId const &,
            TransactionMapKey,
            std::wstring const & type,
            std::wstring const & key) = 0;

        virtual Common::ErrorCode OnCommit(
            Common::ActivityId const &,
            TransactionMapKey) = 0;
    };

    typedef std::weak_ptr<IReplicatedStoreTxEventHandler> IReplicatedStoreTxEventHandlerWPtr;
    typedef std::shared_ptr<IReplicatedStoreTxEventHandler> IReplicatedStoreTxEventHandlerSPtr;
}
