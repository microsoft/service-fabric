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

static const ULONG WAREHOUSE_SL_TAG = 'whsl';

WarehouseValueByteSerializer::WarehouseValueByteSerializer()
{

}

WarehouseValueByteSerializer::~WarehouseValueByteSerializer()
{

}

NTSTATUS WarehouseValueByteSerializer::Create(
	__in KAllocator& allocator,
	__out SPtr& result)
{
	NTSTATUS status;
	SPtr output = _new(WAREHOUSE_SL_TAG, allocator) WarehouseValueByteSerializer();

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

void WarehouseValueByteSerializer::Write(
	__in WarehouseValue::SPtr warehouseValue,
	__in BinaryWriter& binaryWriter)
{
	binaryWriter.Write(*warehouseValue->Name);
	binaryWriter.Write(*warehouseValue->Street_1);
	binaryWriter.Write(*warehouseValue->Street_2);
	binaryWriter.Write(*warehouseValue->City);
	binaryWriter.Write(*warehouseValue->State);
	binaryWriter.Write(*warehouseValue->Zip);
	binaryWriter.Write((LONG64)warehouseValue->Tax);
	binaryWriter.Write((LONG64)warehouseValue->Ytd);
}

WarehouseValue::SPtr WarehouseValueByteSerializer::Read(
	__in BinaryReader& binaryReader)
{
	WarehouseValue::SPtr warehouseValue = nullptr;
	NTSTATUS status = WarehouseValue::Create(this->GetThisAllocator(), warehouseValue);
	KInvariant(NT_SUCCESS(status));
	binaryReader.Read(warehouseValue->Name);
	binaryReader.Read(warehouseValue->Street_1);
	binaryReader.Read(warehouseValue->Street_2);
	binaryReader.Read(warehouseValue->City);
	binaryReader.Read(warehouseValue->State);
	binaryReader.Read(warehouseValue->Zip);
	binaryReader.Read((LONG64&)warehouseValue->Tax);
	binaryReader.Read((LONG64&)warehouseValue->Ytd);
	return warehouseValue;
}
