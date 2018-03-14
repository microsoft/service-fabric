// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define FileCountMergeConfiguration_Tag 'cmCF'

using namespace Data::TStore;

NTSTATUS
FileCountMergeConfiguration::Create(__in ULONG32 mergeThreshold, __in KAllocator& allocator, __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(FileCountMergeConfiguration_Tag, allocator) FileCountMergeConfiguration(mergeThreshold);

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

NTSTATUS
FileCountMergeConfiguration::Create(__in KAllocator& allocator, __out SPtr& result)
{
    return FileCountMergeConfiguration::Create(DefaultMergeThreshold, allocator, result);
}

FileCountMergeConfiguration::FileCountMergeConfiguration(__in ULONG32 mergeThreshold) : fileCountMergeThreshold_(mergeThreshold)
{
}

FileCountMergeConfiguration::~FileCountMergeConfiguration()
{
}

ULONG32 FileCountMergeConfiguration::GetFileType(__in ULONG64 fileSize)
{
    if (fileSize < DefaultVerySmallFileSizeThreshold)
    {
        return 0;
    }

    if (fileSize < DefaultSmallFileSizeThreshold)
    {
        return 1;
    }

    if (fileSize < DefaultMediumFileSizeThreshold)
    {
        return 2;
    }

    if (fileSize < DefaultLargeFileSizeThreshold)
    {
        return 3;
    }

    return 4;
}
