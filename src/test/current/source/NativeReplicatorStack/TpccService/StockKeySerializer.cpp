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

static const ULONG STOCK_KEY_SL_TAG = 'sssl';

StockKeySerializer::StockKeySerializer()
{

}

StockKeySerializer::~StockKeySerializer()
{

}

NTSTATUS StockKeySerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(STOCK_KEY_SL_TAG, allocator) StockKeySerializer();

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

void StockKeySerializer::Write(
    __in StockKey::SPtr stockKey,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(stockKey->Warehouse);
    binaryWriter.Write(stockKey->Stock);
}

StockKey::SPtr StockKeySerializer::Read(
    __in BinaryReader& binaryReader)
{
    StockKey::SPtr stockKey = nullptr;
    NTSTATUS status = StockKey::Create(this->GetThisAllocator(), stockKey);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(stockKey->Warehouse);
    binaryReader.Read(stockKey->Stock);
    return stockKey;
}
