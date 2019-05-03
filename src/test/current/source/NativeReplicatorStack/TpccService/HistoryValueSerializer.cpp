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

static const ULONG HISTORY_SL_TAG = 'hisl';

HistoryValueSerializer::HistoryValueSerializer()
{

}

HistoryValueSerializer::~HistoryValueSerializer()
{

}

NTSTATUS HistoryValueSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(HISTORY_SL_TAG, allocator) HistoryValueSerializer();

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

void HistoryValueSerializer::Write(
    __in HistoryValue::SPtr historyValue,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(historyValue->Warehouse);
    binaryWriter.Write(historyValue->District);
    binaryWriter.Write(historyValue->CustomerWarehouse);
    binaryWriter.Write(historyValue->CustomerDistrict);
    binaryWriter.Write(historyValue->Customer);
    binaryWriter.Write((LONG64)historyValue->Amount);
    binaryWriter.Write(*historyValue->Data);
}

HistoryValue::SPtr HistoryValueSerializer::Read(
    __in BinaryReader& binaryReader)
{
    HistoryValue::SPtr historyValue = nullptr;
    NTSTATUS status = HistoryValue::Create(this->GetThisAllocator(), historyValue);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(historyValue->Warehouse);
    binaryReader.Read(historyValue->District);
    binaryReader.Read(historyValue->CustomerWarehouse);
    binaryReader.Read(historyValue->CustomerDistrict);
    binaryReader.Read(historyValue->Customer);
    binaryReader.Read((LONG64&)historyValue->Amount);
    binaryReader.Read(historyValue->Data);
    return historyValue;
}
