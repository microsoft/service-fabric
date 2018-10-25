// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define LOCK_MODE_TAG 'mmLI'

namespace Data
{
    namespace Utilities
    {
        class LockModeComparer
            : public KObject<LockModeComparer>
            , public KShared<LockModeComparer>
            , public IComparer<LockMode::Enum>
        {
            K_FORCE_SHARED(LockModeComparer)
                K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out IComparer<LockMode::Enum>::SPtr & result);

            int Compare(__in const LockMode::Enum& x, __in const LockMode::Enum& y) const override;
        };
    }
}