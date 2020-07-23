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

static const ULONG STOCK_KEY_TAG = 'stkt';

NTSTATUS StockKey::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(STOCK_KEY_TAG, allocator) StockKey();

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

ULONG StockKey::Hash(__in const StockKey::SPtr& key)
{
    return (ULONG)(key->Warehouse ^ key->Stock);
}

StockKey::StockKey()
    : warehouse_(0)
    , stock_(0)
{

}

StockKey::~StockKey()
{

}
