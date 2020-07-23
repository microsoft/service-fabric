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

static const ULONG ORDERLINE_KEY_TAG = 'olkt';

NTSTATUS OrderLineKey::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(ORDERLINE_KEY_TAG, allocator) OrderLineKey();

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

ULONG OrderLineKey::Hash(__in const OrderLineKey::SPtr& key)
{
    return (ULONG)(key->Warehouse ^ key->District ^ key->Order ^ key->Item ^ key->SupplyWarehouse ^ key->Number);
}

OrderLineKey::OrderLineKey()
    : warehouse_(0)
    , district_(0)
    , order_(0)
    , item_(0)
    , supplyWarehouse_(0)
    , number_(0)
{

}

OrderLineKey::~OrderLineKey()
{

}
