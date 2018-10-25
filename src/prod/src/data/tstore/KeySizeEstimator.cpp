// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::TStore;

NTSTATUS KeySizeEstimator::Create(
    __in KAllocator & allocator,
    __out SPtr & result)
{
    NTSTATUS status;

    SPtr output = _new(KEY_SIZE_ESTIMATOR_TAG, allocator) KeySizeEstimator();

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

KeySizeEstimator::KeySizeEstimator() 
    : numKeys_(0)
    , totalSize_(0)
{
}

KeySizeEstimator::~KeySizeEstimator()
{
}

void KeySizeEstimator::SetInitialEstimate(__in LONG64 estimate)
{
    initialEstimate_ = estimate;
}

void KeySizeEstimator::AddKeySize(__in LONG64 totalSize, __in LONG64 count)
{
    initialEstimate_ = -1;
    InterlockedAdd64(&totalSize_, totalSize);
    InterlockedAdd64(&numKeys_, count);
}

LONG64 KeySizeEstimator::GetEstimatedKeySize()
{
    if (initialEstimate_ != -1)
    {
        return initialEstimate_;
    }

    return numKeys_ > 0 ? totalSize_ / numKeys_ : 0;
}