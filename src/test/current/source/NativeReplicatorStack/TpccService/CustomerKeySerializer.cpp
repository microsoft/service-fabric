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

static const ULONG CUSTOMER_KEY_SL_TAG = 'cksl';

CustomerKeySerializer::CustomerKeySerializer()
{

}

CustomerKeySerializer::~CustomerKeySerializer()
{

}

NTSTATUS CustomerKeySerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(CUSTOMER_KEY_SL_TAG, allocator) CustomerKeySerializer();

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

void CustomerKeySerializer::Write(
    __in CustomerKey::SPtr customerKey,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(customerKey->Warehouse);
    binaryWriter.Write(customerKey->District);
    binaryWriter.Write(customerKey->Customer);
}

CustomerKey::SPtr CustomerKeySerializer::Read(
    __in BinaryReader& binaryReader)
{
    CustomerKey::SPtr customerKey = nullptr;
    NTSTATUS status = CustomerKey::Create(this->GetThisAllocator(), customerKey);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(customerKey->Warehouse);
    binaryReader.Read(customerKey->District);
    binaryReader.Read(customerKey->Customer);
    return customerKey;
}
