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

static const ULONG WAREHOUSE_VAL_TAG = 'whvt';

NTSTATUS WarehouseValue::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(WAREHOUSE_VAL_TAG, allocator) WarehouseValue();

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

WarehouseValue::WarehouseValue()
    : name_(nullptr)
    , street1_(nullptr)
    , street2_(nullptr)
    , city_(nullptr)
    , state_(nullptr)
    , zip_(nullptr)
    , tax_(0)
    , ytd_(0)
{
    KString::Create(name_, this->GetThisAllocator(), L"default");
    KString::Create(street1_, this->GetThisAllocator(), L"default");
    KString::Create(street2_, this->GetThisAllocator(), L"default");
    KString::Create(city_, this->GetThisAllocator(), L"default");
    KString::Create(state_, this->GetThisAllocator(), L"default");
    KString::Create(zip_, this->GetThisAllocator(), L"default");
}

WarehouseValue::~WarehouseValue()
{

}
