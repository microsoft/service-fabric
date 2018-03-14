// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Data::TStore;

NullableStringStateSerializer::NullableStringStateSerializer(Data::Utilities::Encoding encoding) : encoding_(encoding)
{
}

NullableStringStateSerializer::~NullableStringStateSerializer()
{
}

NTSTATUS NullableStringStateSerializer::Create(
    __in KAllocator & allocator,
    __out NullableStringStateSerializer::SPtr & result,
    __in_opt Data::Utilities::Encoding encoding)
{
    result = _new(NULLABLE_STRING_SERIALIZER_TAG, allocator) NullableStringStateSerializer(encoding);

    if (!result)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

void NullableStringStateSerializer::Write(
    __in KString::SPtr value,
    __in BinaryWriter& binaryWriter)
{
    if (value == nullptr)
    {
        binaryWriter.Write(false);
        return;
    }

    binaryWriter.Write(true);
    binaryWriter.Write(*value, encoding_);
}

KString::SPtr NullableStringStateSerializer::Read(__in BinaryReader& binaryReader)
{
    bool hasValue;
    binaryReader.Read(hasValue);
    if (hasValue == false)
    {
        return nullptr;
    }

    KString::SPtr value;
    binaryReader.Read(value, encoding_);
    return value;
}
