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

static const ULONG NEWORDER_KEY_SL_TAG = 'nksl';

NewOrderKeySerializer::NewOrderKeySerializer()
{

}

NewOrderKeySerializer::~NewOrderKeySerializer()
{

}

NTSTATUS NewOrderKeySerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(NEWORDER_KEY_SL_TAG, allocator) NewOrderKeySerializer();

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

void NewOrderKeySerializer::Write(
    __in NewOrderKey::SPtr newOrderKey,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(newOrderKey->Warehouse);
    binaryWriter.Write(newOrderKey->District);
    binaryWriter.Write(newOrderKey->Order);
}

NewOrderKey::SPtr NewOrderKeySerializer::Read(
    __in BinaryReader& binaryReader)
{
    NewOrderKey::SPtr newOrderKey = nullptr;
    NTSTATUS status = NewOrderKey::Create(this->GetThisAllocator(), newOrderKey);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(newOrderKey->Warehouse);
    binaryReader.Read(newOrderKey->District);
    binaryReader.Read(newOrderKey->Order);
    return newOrderKey;
}
