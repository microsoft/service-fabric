// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::Utilities;

BinaryReader::BinaryReader(
    __in KBuffer const & readBuffer,
    __in KAllocator & allocator)
    : KObject()
    , in_(allocator)
    , memChannel_(allocator)
    , allocator_(allocator)
{
    memChannel_.SetReadFullRequired(TRUE); // Ensures partial reads fails the call

    // Write the content into the channel and reset the cursor
    NTSTATUS status = memChannel_.Write(readBuffer.QuerySize(), readBuffer.GetBuffer());
    THROW_ON_FAILURE(status);

    status = memChannel_.ResetCursor();
    THROW_ON_FAILURE(status);
}

void BinaryReader::Read(__out KBuffer::SPtr & value)
{
    LONG32 length;
    Read(length);

    ASSERT_IFNOT(length >= NULL_KBUFFER_SERIALIZATION_CODE, "Invalid read length in binary reader");

    if (length == NULL_KBUFFER_SERIALIZATION_CODE)
    {
        value = nullptr;
        return;
    }

    NTSTATUS status = KBuffer::Create(length, value, allocator_);
    THROW_ON_FAILURE(status);

    status = memChannel_.Read(length, value->GetBuffer());
    THROW_ON_FAILURE(status);
}
