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

static const ULONG HISTORY_VAL_TAG = 'hivt';

NTSTATUS HistoryValue::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(HISTORY_VAL_TAG, allocator) HistoryValue();

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

HistoryValue::HistoryValue()
    : warehouse_(0)
    , district_(0)
    , customerWarehouse_(0)
    , customerDistrict_(0)
    , customer_(0)
    , amount_(0)
    , data_(nullptr)
{
    KString::Create(data_, this->GetThisAllocator(), L"default");
}

HistoryValue::~HistoryValue()
{

}
