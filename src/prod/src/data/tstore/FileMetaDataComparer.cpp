// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data;
using namespace Data::Utilities;

FileMetaDataComparer::FileMetaDataComparer()
{
}

FileMetaDataComparer::~FileMetaDataComparer()
{
}

NTSTATUS FileMetaDataComparer::Create(
    __in KAllocator & allocator,
    __out FileMetaDataComparer::SPtr & result)
{
    result = _new(FILEMETA_COMPARER_TAG, allocator) FileMetaDataComparer();

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

int FileMetaDataComparer::Compare(__in const FileMetadata::SPtr & keyOne, __in const FileMetadata::SPtr & keyTwo) const
{
    KInvariant(keyOne != nullptr);
    KInvariant(keyTwo != nullptr);

    return keyOne->FileId < keyTwo->FileId ? -1 : keyOne->FileId > keyTwo->FileId ? 1 : 0;
}
