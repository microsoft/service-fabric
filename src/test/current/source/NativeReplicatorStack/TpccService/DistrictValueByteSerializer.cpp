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

static const ULONG DISTRICT_SL_TAG = 'disl';

DistrictValueByteSerializer::DistrictValueByteSerializer()
{

}

DistrictValueByteSerializer::~DistrictValueByteSerializer()
{

}

NTSTATUS DistrictValueByteSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(DISTRICT_SL_TAG, allocator) DistrictValueByteSerializer();

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

void DistrictValueByteSerializer::Write(
    __in DistrictValue::SPtr districtValue,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(*districtValue->Name);
    binaryWriter.Write(*districtValue->Street_1);
    binaryWriter.Write(*districtValue->Street_2);
    binaryWriter.Write(*districtValue->City);
    binaryWriter.Write(*districtValue->State);
    binaryWriter.Write(*districtValue->Zip);
    binaryWriter.Write((LONG64)districtValue->Tax);
    binaryWriter.Write((LONG64)districtValue->Ytd);
    binaryWriter.Write(districtValue->NextOrderId);
}

DistrictValue::SPtr DistrictValueByteSerializer::Read(
    __in BinaryReader& binaryReader)
{
    DistrictValue::SPtr districtValue = nullptr;
    NTSTATUS status = DistrictValue::Create(this->GetThisAllocator(), districtValue);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(districtValue->Name);
    binaryReader.Read(districtValue->Street_1);
    binaryReader.Read(districtValue->Street_2);
    binaryReader.Read(districtValue->City);
    binaryReader.Read(districtValue->State);
    binaryReader.Read(districtValue->Zip);
    binaryReader.Read((LONG64&)districtValue->Tax);
    binaryReader.Read((LONG64&)districtValue->Ytd);
    binaryReader.Read(districtValue->NextOrderId);
    return districtValue;
}
