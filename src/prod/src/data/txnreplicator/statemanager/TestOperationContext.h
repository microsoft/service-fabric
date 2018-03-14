// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace StateManagerTests
{
    class TestOperationContext :
        public TxnReplicator::OperationContext
    {
        K_FORCE_SHARED_WITH_INHERITANCE(TestOperationContext)

    public:
        static SPtr Create(__in LONG64 stateProviderId, __in KAllocator & allocator) noexcept;

    public:
        __declspec(property(get = get_IsUnlocked)) bool IsUnlocked;
        bool get_IsUnlocked() const;

    public:
        void Unlock(__in LONG64 stateProviderId) noexcept;

    private:
        NOFAIL TestOperationContext(__in LONG64 stateProviderId) noexcept;

    private:
        const LONG64 stateProviderId_;
        bool isUnlocked_ = false;
    };
}
