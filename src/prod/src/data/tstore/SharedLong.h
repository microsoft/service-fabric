// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define SHARED_LONG_TAG 'tcLS'

namespace TStoreTests
{
    class SharedLong
        : public KObject<SharedLong>
        , public KShared<SharedLong>
    {
        K_FORCE_SHARED(SharedLong)
    public:
        static SharedLong::SPtr Create(
            __in LONG64 value,
            __in KAllocator & allocator);

        __declspec(property(get = get_Value)) LONG64 Value;
        LONG64 get_Value()
        {
            return value_;
        }

    private:
        SharedLong(__in LONG64 value);

        LONG64 value_;
    };

    class SharedLongComparer
        : public KObject<SharedLongComparer>
        , public KShared<SharedLongComparer>
        , public IComparer<SharedLong::SPtr>
    {
        K_FORCE_SHARED(SharedLongComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

    public:
        static IComparer<SharedLong::SPtr>::SPtr Create(__in KAllocator & allocator);

        int Compare(
            __in const SharedLong::SPtr & x, 
            __in const SharedLong::SPtr & y) const override;
    };
}
