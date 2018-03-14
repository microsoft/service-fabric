// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestVersionProvider final
        : public TxnReplicator::IVersionProvider
        , public KObject<TestVersionProvider>
        , public KShared<TestVersionProvider>
        , public KWeakRefType<TestVersionProvider>
    {
        K_FORCE_SHARED(TestVersionProvider)
        K_SHARED_INTERFACE_IMP(IVersionProvider)
        K_WEAKREF_INTERFACE_IMP(IVersionProvider, TestVersionProvider)

    public:
        static SPtr Create(
            __in std::vector<LONG64> & resultList,
            __in KAllocator& allocator);

        NTSTATUS GetVersion(__out LONG64 & version) const noexcept override;

    private:
        TestVersionProvider(std::vector<LONG64> & resultList) noexcept;

        mutable vector<LONG64>::iterator barrierIterator_;
        vector<LONG64>::iterator barrierIteratorEnd_;
    };
}
