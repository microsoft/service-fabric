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

static const ULONG ORDERLINE_VAL_SL_TAG = 'lvsl';

OrderLineValueSerializer::OrderLineValueSerializer()
{

}

OrderLineValueSerializer::~OrderLineValueSerializer()
{

}

NTSTATUS OrderLineValueSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(ORDERLINE_VAL_SL_TAG, allocator) OrderLineValueSerializer();

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

void OrderLineValueSerializer::Write(
    __in OrderLineValue::SPtr orderLineValue,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(orderLineValue->Quantity);
    binaryWriter.Write((LONG64)orderLineValue->Amount);
    binaryWriter.Write(*orderLineValue->Info);
}

OrderLineValue::SPtr OrderLineValueSerializer::Read(
    __in BinaryReader& binaryReader)
{
    OrderLineValue::SPtr orderLineValue = nullptr;
    NTSTATUS status = OrderLineValue::Create(this->GetThisAllocator(), orderLineValue);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(orderLineValue->Quantity);
    binaryReader.Read((LONG64&)orderLineValue->Amount);
    binaryReader.Read(orderLineValue->Info);
    return orderLineValue;
}
