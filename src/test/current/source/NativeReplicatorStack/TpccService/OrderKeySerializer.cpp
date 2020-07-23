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

static const ULONG ORDER_KEY_SL_TAG = 'oksl';

OrderKeySerializer::OrderKeySerializer()
{

}

OrderKeySerializer::~OrderKeySerializer()
{

}

NTSTATUS OrderKeySerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(ORDER_KEY_SL_TAG, allocator) OrderKeySerializer();

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

void OrderKeySerializer::Write(
    __in OrderKey::SPtr orderKey,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(orderKey->Warehouse);
    binaryWriter.Write(orderKey->District);
    binaryWriter.Write(orderKey->Customer);
    binaryWriter.Write(orderKey->Order);
}

OrderKey::SPtr OrderKeySerializer::Read(
    __in BinaryReader& binaryReader)
{
    OrderKey::SPtr orderKey = nullptr;
    NTSTATUS status = OrderKey::Create(this->GetThisAllocator(), orderKey);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(orderKey->Warehouse);
    binaryReader.Read(orderKey->District);
    binaryReader.Read(orderKey->Customer);
    binaryReader.Read(orderKey->Order);
    return orderKey;
}
