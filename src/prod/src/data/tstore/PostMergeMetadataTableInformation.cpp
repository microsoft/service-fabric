// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;

NTSTATUS PostMergeMetadataTableInformation::Create(
    __in KSharedArray<ULONG32> & deletedFileIds,
    __in FileMetadata::SPtr newMergedFileSPtr, // Can be null
    __in KAllocator & allocator,
    __out SPtr & result)
{
    NTSTATUS status;
    SPtr output = _new(POSTMERGEMETADATAINFO_TAG, allocator) PostMergeMetadataTableInformation(deletedFileIds, newMergedFileSPtr);

    if (!output)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = output->Status();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    result = Ktl::Move(output);
    return STATUS_SUCCESS;
}

PostMergeMetadataTableInformation::PostMergeMetadataTableInformation(
    __in KSharedArray<ULONG32> & deletedFileIds, 
    __in FileMetadata::SPtr  newMergedFileSPtr) // Can be null
    : deletedFileIdsSPtr_(&deletedFileIds),
    newMergedFileSPtr_(newMergedFileSPtr)
{
}

PostMergeMetadataTableInformation::~PostMergeMetadataTableInformation()
{
}
