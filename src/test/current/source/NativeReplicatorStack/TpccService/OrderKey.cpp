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

static const ULONG ORDER_KEY_TAG = 'orkt';

NTSTATUS OrderKey::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(ORDER_KEY_TAG, allocator) OrderKey();

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

ULONG OrderKey::Hash(__in const OrderKey::SPtr& key)
{
    return (ULONG)(key->Warehouse ^ key->District ^ key->Customer ^ key->Order);
}

OrderKey::OrderKey()
    : warehouse_(0)
    , district_(0)
    , customer_(0)
    , order_(0)
{

}

OrderKey::~OrderKey()
{

}
