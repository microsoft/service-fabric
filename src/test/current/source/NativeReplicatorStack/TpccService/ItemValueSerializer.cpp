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

static const ULONG ITEM_VAL_SL_TAG = 'ivsl';

ItemValueSerializer::ItemValueSerializer()
{

}

ItemValueSerializer::~ItemValueSerializer()
{

}

NTSTATUS ItemValueSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(ITEM_VAL_SL_TAG, allocator) ItemValueSerializer();

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

void ItemValueSerializer::Write(
    __in ItemValue::SPtr itemValue,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(itemValue->Image);
    binaryWriter.Write(*itemValue->Name);
    binaryWriter.Write((LONG64)itemValue->Price);
    binaryWriter.Write(*itemValue->Data);
}

ItemValue::SPtr ItemValueSerializer::Read(
    __in BinaryReader& binaryReader)
{
    ItemValue::SPtr itemValue = nullptr;
    NTSTATUS status = ItemValue::Create(this->GetThisAllocator(), itemValue);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(itemValue->Image);
    binaryReader.Read(itemValue->Name);
    binaryReader.Read((LONG64&)itemValue->Price);
    binaryReader.Read(itemValue->Data);
    return itemValue;
}
