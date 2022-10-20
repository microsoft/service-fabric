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

static const ULONG LONG64_SL_TAG = 'losl';

Long64Serializer::Long64Serializer()
{
}

Long64Serializer::~Long64Serializer()
{
}

NTSTATUS Long64Serializer::Create(
    __in KAllocator& allocator,
    __out SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(LONG64_SL_TAG, allocator) Long64Serializer();

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

void Long64Serializer::Write(
    __in LONG64 value,
    __in BinaryWriter& binaryWriter)
{
    binaryWriter.Write(value);
}

LONG64 Long64Serializer::Read(
    __in BinaryReader& binaryReader)
{
    LONG64 value = 0;
    binaryReader.Read(value);
    return value;
}
