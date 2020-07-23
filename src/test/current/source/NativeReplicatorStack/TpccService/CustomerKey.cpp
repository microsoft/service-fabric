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

static const ULONG CUSTOMER_KEY_TAG = 'cukt';

NTSTATUS CustomerKey::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(CUSTOMER_KEY_TAG, allocator) CustomerKey();

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

ULONG CustomerKey::Hash(__in const CustomerKey::SPtr& key)
{
    return key->Warehouse ^ key->District ^ key->Customer;
}

CustomerKey::CustomerKey()
    : warehouse_(0)
    , district_(0)
    , customer_(0)
{

}

CustomerKey::~CustomerKey()
{

}
