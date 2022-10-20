// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //
    // This is the pool header for the pool blocks allocated by InterlockedBufferPool
    //
    struct InterlockedBufferPoolItem
    {

        InterlockedBufferPoolItem();
        ~InterlockedBufferPoolItem();

        InterlockedBufferPoolItem* pNext_;
        BYTE buffer_[1];
    };

    typedef InterlockedPool<InterlockedBufferPoolItem> InterlockedBufferPool;
}
