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

static const ULONG ORDER_VAL_SL_TAG = 'ovsl';

OrderValueSerializer::OrderValueSerializer()
{

}

OrderValueSerializer::~OrderValueSerializer()
{

}

NTSTATUS OrderValueSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(ORDER_VAL_SL_TAG, allocator) OrderValueSerializer();

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

void OrderValueSerializer::Write(
    __in OrderValue::SPtr orderValue,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(orderValue->Carrier);
    binaryWriter.Write(orderValue->LineCount);
    binaryWriter.Write(orderValue->Local);
}

OrderValue::SPtr OrderValueSerializer::Read(
    __in BinaryReader& binaryReader)
{
    OrderValue::SPtr orderValue = nullptr;
    NTSTATUS status = OrderValue::Create(this->GetThisAllocator(), orderValue);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(orderValue->Carrier);
    binaryReader.Read(orderValue->LineCount);
    binaryReader.Read(orderValue->Local);
    return orderValue;
}
