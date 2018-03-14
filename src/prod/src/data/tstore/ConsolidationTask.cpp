// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;

NTSTATUS ConsolidationTask::Create(
    __in KAllocator & allocator,
    __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(COPY_MANAGER_TAG, allocator) ConsolidationTask();

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

ConsolidationTask::ConsolidationTask()
{
}

ConsolidationTask::~ConsolidationTask()
{
}

ktl::Awaitable<PostMergeMetadataTableInformation::SPtr> ConsolidationTask::RunAsync()
{
    PostMergeMetadataTableInformation::SPtr postMergeInfo =  co_await awaitable_;
    co_return postMergeInfo;
}
