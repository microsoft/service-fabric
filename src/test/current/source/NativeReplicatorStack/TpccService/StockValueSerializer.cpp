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

static const ULONG STOCK_VAL_SL_TAG = 'svsl';

StockValueSerializer::StockValueSerializer()
{

}

StockValueSerializer::~StockValueSerializer()
{

}

NTSTATUS StockValueSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(STOCK_VAL_SL_TAG, allocator) StockValueSerializer();

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

void StockValueSerializer::Write(
    __in StockValue::SPtr stockValue,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(stockValue->Quantity);
    binaryWriter.Write(*(stockValue->District1));
    binaryWriter.Write(*(stockValue->District2));
    binaryWriter.Write(*(stockValue->District3));
    binaryWriter.Write(*(stockValue->District4));
    binaryWriter.Write(*(stockValue->District5));
    binaryWriter.Write(*(stockValue->District6));
    binaryWriter.Write(*(stockValue->District7));
    binaryWriter.Write(*(stockValue->District8));
    binaryWriter.Write(*(stockValue->District9));
    binaryWriter.Write(*(stockValue->District10));
    binaryWriter.Write((LONG64)stockValue->Ytd);
    binaryWriter.Write(stockValue->OrderCount);
    binaryWriter.Write(stockValue->RemoteCount);
    binaryWriter.Write(*(stockValue->Data));
}

StockValue::SPtr StockValueSerializer::Read(
    __in BinaryReader& binaryReader)
{
    StockValue::SPtr stockValue = nullptr;
    NTSTATUS status = StockValue::Create(this->GetThisAllocator(), stockValue);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(stockValue->Quantity);
    binaryReader.Read(stockValue->District1);
    binaryReader.Read(stockValue->District2);
    binaryReader.Read(stockValue->District3);
    binaryReader.Read(stockValue->District4);
    binaryReader.Read(stockValue->District5);
    binaryReader.Read(stockValue->District6);
    binaryReader.Read(stockValue->District7);
    binaryReader.Read(stockValue->District8);
    binaryReader.Read(stockValue->District9);
    binaryReader.Read(stockValue->District10);
    binaryReader.Read((LONG64&)stockValue->Ytd);
    binaryReader.Read(stockValue->OrderCount);
    binaryReader.Read(stockValue->RemoteCount);
    binaryReader.Read(stockValue->Data);
    return stockValue;
}
