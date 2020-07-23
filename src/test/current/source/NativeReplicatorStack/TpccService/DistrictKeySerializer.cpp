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

static const ULONG DISTRICT_KEY_SL_TAG = 'dksl';

DistrictKeySerializer::DistrictKeySerializer()
{

}

DistrictKeySerializer::~DistrictKeySerializer()
{

}

NTSTATUS DistrictKeySerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(DISTRICT_KEY_SL_TAG, allocator) DistrictKeySerializer();

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

void DistrictKeySerializer::Write(
    __in DistrictKey::SPtr districtKey,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(districtKey->Warehouse);
    binaryWriter.Write(districtKey->District);
}

DistrictKey::SPtr DistrictKeySerializer::Read(
    __in BinaryReader& binaryReader)
{
    DistrictKey::SPtr districtKey = nullptr;
    NTSTATUS status = DistrictKey::Create(this->GetThisAllocator(), districtKey);
    KInvariant(NT_SUCCESS(status));
    binaryReader.Read(districtKey->Warehouse);
    binaryReader.Read(districtKey->District);
    return districtKey;
}
