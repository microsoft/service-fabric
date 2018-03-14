// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// Copyright (c) 2009 Microsoft Corporation
#include "stdafx.h"
#include "Common/VectorStream.h"

namespace Common
{
    void VectorStream::WriteBytes(void const * buf, size_t size)
    {
        ASSERT_IF(isReadonly_, "Cannot write to a const vector<byte>.  Must use non-const constructor.");

        size_t oldSize = iterator_ - vector_.begin();
        byte const * begin = reinterpret_cast<byte const*>(buf);
        vector_.insert(iterator_, begin, begin + size);
        iterator_ = vector_.begin() + oldSize + size;
    }

    void VectorStream::ReadBytes(void * buf, size_t size)
    {
        ASSERT_IF(iterator_ + size > vector_.end(), "Read beyond the vector");
        copy_n_checked(&(*iterator_), size, static_cast<byte*>(buf), size); 
        iterator_ += size;
    }

    bool VectorStream::empty()
    {
        return (iterator_ == vector_.begin());
    }
}
