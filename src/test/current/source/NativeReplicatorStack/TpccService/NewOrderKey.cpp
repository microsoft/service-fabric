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

static const ULONG NEWORDER_KEY_TAG = 'nokt';

NTSTATUS NewOrderKey::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(NEWORDER_KEY_TAG, allocator) NewOrderKey();

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

ULONG NewOrderKey::Hash(__in const NewOrderKey::SPtr& key)
{
    return (ULONG)(key->Warehouse ^ key->District ^ key->Order);
}

NewOrderKey::NewOrderKey()
    : warehouse_(0)
    , district_(0)
    , order_(0)
{

}

NewOrderKey::~NewOrderKey()
{

}
