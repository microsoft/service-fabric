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

static const StringLiteral TraceComponent("GuidComparer");

static const ULONG GUID_CMP_TAG = 'guct';

NTSTATUS GuidComparer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(GUID_CMP_TAG, allocator) GuidComparer();

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

int GuidComparer::Compare(
    __in const KGuid& lhs,
    __in const KGuid& rhs) const
{
    Guid guid1(lhs);
    Guid guid2(rhs);
    if (guid1 < guid2) {
        return -1;
    }
    else if (guid1 > guid2) {
        return 1;
    }
    else {
        return 0;
    }
}

GuidComparer::GuidComparer()
{
}

GuidComparer::~GuidComparer()
{
}
