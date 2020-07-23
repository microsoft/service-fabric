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

static const ULONG NEWORDER_VAL_SL_TAG = 'nvsl';

NewOrderValueSerializer::NewOrderValueSerializer()
{

}

NewOrderValueSerializer::~NewOrderValueSerializer()
{

}

NTSTATUS NewOrderValueSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(NEWORDER_VAL_SL_TAG, allocator) NewOrderValueSerializer();

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

void NewOrderValueSerializer::Write(
    __in NewOrderValue::SPtr newOrderValue,
    __in BinaryWriter& binaryWriter)
{
    UNREFERENCED_PARAMETER(newOrderValue);
    binaryWriter.Write(-1);
}

NewOrderValue::SPtr NewOrderValueSerializer::Read(
    __in BinaryReader& binaryReader)
{
    NewOrderValue::SPtr newOrderValue = nullptr;
    NTSTATUS status = NewOrderValue::Create(this->GetThisAllocator(), newOrderValue);
    KInvariant(NT_SUCCESS(status));
    int temp;
    binaryReader.Read(temp);
    return newOrderValue;
}
