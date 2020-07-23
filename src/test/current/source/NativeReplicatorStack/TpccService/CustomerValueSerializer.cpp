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

static const ULONG CUSTOMER_SL_TAG = 'cusl';

CustomerValueSerializer::CustomerValueSerializer()
{

}

CustomerValueSerializer::~CustomerValueSerializer()
{

}

NTSTATUS CustomerValueSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(CUSTOMER_SL_TAG, allocator) CustomerValueSerializer();

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

void CustomerValueSerializer::Write(
    __in CustomerValue::SPtr customerValue,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(*customerValue->FirstName);
    binaryWriter.Write(*customerValue->MiddleName);
    binaryWriter.Write(*customerValue->LastName);
    binaryWriter.Write(*customerValue->Street_1);
    binaryWriter.Write(*customerValue->Street_2);
    binaryWriter.Write(*customerValue->City);
    binaryWriter.Write(*customerValue->State);
    binaryWriter.Write(*customerValue->Zip);
    binaryWriter.Write(*customerValue->Phone);
	// TODO: support Datetime
    // binaryWriter.Write(customerValue->Since);
    binaryWriter.Write(*customerValue->Credit);
    binaryWriter.Write((LONG64)customerValue->CreditLimit);
    binaryWriter.Write((LONG64)customerValue->Discount);
    binaryWriter.Write((LONG64)customerValue->Balance);
    binaryWriter.Write((LONG64)customerValue->Ytd);
    binaryWriter.Write(customerValue->PaymentCount);
    binaryWriter.Write(customerValue->DeliveryCount);
    binaryWriter.Write(customerValue->Data);
}

CustomerValue::SPtr CustomerValueSerializer::Read(
    __in BinaryReader& binaryReader)
{
    CustomerValue::SPtr customerValue = nullptr;
    NTSTATUS status = CustomerValue::Create(this->GetThisAllocator(), customerValue);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(customerValue->FirstName);
    binaryReader.Read(customerValue->MiddleName);
    binaryReader.Read(customerValue->LastName);
    binaryReader.Read(customerValue->Street_1);
    binaryReader.Read(customerValue->Street_2);
    binaryReader.Read(customerValue->City);
    binaryReader.Read(customerValue->State);
    binaryReader.Read(customerValue->Zip);
    binaryReader.Read(customerValue->Phone);
	// TODO: support Datetime
    // binaryReader.Read(customerValue->Since);
    binaryReader.Read(customerValue->Credit);
    binaryReader.Read((LONG64&)customerValue->CreditLimit);
    binaryReader.Read((LONG64&)customerValue->Discount);
    binaryReader.Read((LONG64&)customerValue->Balance);
    binaryReader.Read((LONG64&)customerValue->Ytd);
    binaryReader.Read(customerValue->PaymentCount);
    binaryReader.Read(customerValue->DeliveryCount);
    binaryReader.Read(customerValue->Data);
    return customerValue;
}
