// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::LoggingReplicator;
using namespace ktl;

NTSTATUS NotificationKeyComparer::Create(__in KAllocator & allocator, __out SPtr & result) noexcept
{
    result = _new(NOTIFICATION_KEY_COMPARER_TAG, allocator) NotificationKeyComparer();

    if (result == nullptr)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(result->Status()))
    {
        return (SPtr(Ktl::Move(result)))->Status();
    }

    return STATUS_SUCCESS;
}

int NotificationKeyComparer::Compare(__in const NotificationKey::SPtr & x, __in const NotificationKey::SPtr & y) const
{
    if (x->StateProviderId == y->StateProviderId && x->VisibilitySeqeuenceNumber == y->VisibilitySeqeuenceNumber)
    {
        return 0;
    }

    if (x->StateProviderId < y->StateProviderId)
    {
        return -1;
    }

    if (x->StateProviderId > y->StateProviderId)
    {
        return 1;
    }

    if (x->VisibilitySeqeuenceNumber < y->VisibilitySeqeuenceNumber)
    {
        return -1;
    }

    return 1;
}

NotificationKeyComparer::NotificationKeyComparer()
{
}

NotificationKeyComparer::~NotificationKeyComparer()
{
}
