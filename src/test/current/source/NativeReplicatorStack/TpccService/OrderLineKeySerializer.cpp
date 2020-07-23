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

static const ULONG ORDERLINE_KEY_SL_TAG = 'lksl';

OrderLineKeySerializer::OrderLineKeySerializer()
{

}

OrderLineKeySerializer::~OrderLineKeySerializer()
{

}

NTSTATUS OrderLineKeySerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(ORDERLINE_KEY_SL_TAG, allocator) OrderLineKeySerializer();

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

void OrderLineKeySerializer::Write(
    __in OrderLineKey::SPtr orderLineKey,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(orderLineKey->Warehouse);
    binaryWriter.Write(orderLineKey->District);
    binaryWriter.Write(orderLineKey->Order);
    binaryWriter.Write(orderLineKey->Item);
    binaryWriter.Write(orderLineKey->SupplyWarehouse);
    binaryWriter.Write(orderLineKey->Number);
}

OrderLineKey::SPtr OrderLineKeySerializer::Read(
    __in BinaryReader& binaryReader)
{
    OrderLineKey::SPtr orderLineKey = nullptr;
    NTSTATUS status = OrderLineKey::Create(this->GetThisAllocator(), orderLineKey);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(orderLineKey->Warehouse);
    binaryReader.Read(orderLineKey->District);
    binaryReader.Read(orderLineKey->Order);
    binaryReader.Read(orderLineKey->Item);
    binaryReader.Read(orderLineKey->SupplyWarehouse);
    binaryReader.Read(orderLineKey->Number);
    return orderLineKey;
}
