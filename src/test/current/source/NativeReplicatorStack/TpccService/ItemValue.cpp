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

static const ULONG ITEM_VAL_TAG = 'itvt';

NTSTATUS ItemValue::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(ITEM_VAL_TAG, allocator) ItemValue();

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

ItemValue::ItemValue()
    : image_(0)
    , name_(nullptr)
    , price_(0)
    , data_(nullptr)
{
    KString::Create(name_, this->GetThisAllocator(), L"default");
    KString::Create(data_, this->GetThisAllocator(), L"default");
}

ItemValue::~ItemValue()
{
}
