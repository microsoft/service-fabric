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

static const ULONG STOCK_VAL_TAG = 'stvt';

NTSTATUS StockValue::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(STOCK_VAL_TAG, allocator) StockValue();

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

StockValue::StockValue()
    : quantity_(0)
    , district1_(nullptr)
    , district2_(nullptr)
    , district3_(nullptr)
    , district4_(nullptr)
    , district5_(nullptr)
    , district6_(nullptr)
    , district7_(nullptr)
    , district8_(nullptr)
    , district9_(nullptr)
    , district10_(nullptr)
    , ytd_(0)
    , orderCount_(0)
    , remoteCount_(0)
    , data_(nullptr)
{
    KString::Create(district1_, this->GetThisAllocator(), L"default");
    KString::Create(district2_, this->GetThisAllocator(), L"default");
    KString::Create(district3_, this->GetThisAllocator(), L"default");
    KString::Create(district4_, this->GetThisAllocator(), L"default");
    KString::Create(district5_, this->GetThisAllocator(), L"default");
    KString::Create(district6_, this->GetThisAllocator(), L"default");
    KString::Create(district7_, this->GetThisAllocator(), L"default");
    KString::Create(district8_, this->GetThisAllocator(), L"default");
    KString::Create(district9_, this->GetThisAllocator(), L"default");
    KString::Create(district10_, this->GetThisAllocator(), L"default");
	KString::Create(data_, this->GetThisAllocator(), L"default");
}

StockValue::~StockValue()
{

}
