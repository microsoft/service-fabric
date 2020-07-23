// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TXRStatefulServiceBase;
using namespace TpccService;
using namespace TxnReplicator;
using namespace Data::Utilities;

static const StringLiteral TraceComponent("DistrictKeyComparer");
static const ULONG DISTRICT_KEY_CMP_TAG = 'dkct';

NTSTATUS DistrictKeyComparer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(DISTRICT_KEY_CMP_TAG, allocator) DistrictKeyComparer();

    if (output == nullptr)
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

int DistrictKeyComparer::Compare(
    __in const DistrictKey::SPtr& lhs,
    __in const DistrictKey::SPtr& rhs) const
{
    if (lhs == nullptr || rhs == nullptr) {
        Trace.WriteInfo(TraceComponent, "DistrictKey::SPtr cannot be NULL");
        KInvariant(false);
    }

    if (lhs->Warehouse < rhs->Warehouse)
    {
        return -1;
    }
    else if (lhs->Warehouse > rhs->Warehouse)
    {
        return 1;
    }
    else if (lhs->District < rhs->District)
    {
        return -1;
    }
    else if (lhs->District > rhs->District)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

DistrictKeyComparer::DistrictKeyComparer()
{
}

DistrictKeyComparer::~DistrictKeyComparer()
{
}
