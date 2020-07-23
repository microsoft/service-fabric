// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TStoreNightWatchTXRService
{
    class TStoreTestRunner
        : public NightWatchTXRService::TestRunner
    {
        //K_FORCE_SHARED(TStoreTestRunner)
        K_FORCE_SHARED_WITH_INHERITANCE(TStoreTestRunner)
        K_SHARED_INTERFACE_IMP(ITestRunner);

    public:
        static TStoreTestRunner::SPtr Create(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator,
            __in KAllocator & allocator);

    protected:
        virtual ktl::Awaitable<LONG64> AddTx(__in std::wstring const & key);
        virtual ktl::Awaitable<LONG64> UpdateTx(__in std::wstring const & key, __in LONG64 version);

    private:
        TStoreTestRunner(
            __in Common::ByteBufferUPtr const & testParametersJson,
            __in TxnReplicator::ITransactionalReplicator & txnReplicator);

        NightWatchTXRService::NightWatchTestParameters testParameters_;

        KSharedPtr<Data::TStore::IStore<LONG64, KBuffer::SPtr>> storeSPtr_;
        
        // StateProvider-Key map
        KReaderWriterSpinLock keyMapLock_;
        std::unordered_map<std::wstring, LONG64> keyMap_;
        LONG64 index_;
    };
}
