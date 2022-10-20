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

static const ULONG GUID_SL_TAG = 'gusl';

GuidSerializer::GuidSerializer()
{

}

GuidSerializer::~GuidSerializer()
{

}

NTSTATUS GuidSerializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(GUID_SL_TAG, allocator) GuidSerializer();

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

void GuidSerializer::Write(
    __in KGuid guid,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(guid);
}

KGuid GuidSerializer::Read(
    __in BinaryReader& binaryReader)
{
    KGuid guid;
    binaryReader.Read(guid);
    return guid;
}
