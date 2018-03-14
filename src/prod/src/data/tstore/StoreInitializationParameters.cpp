// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Data::TStore;
using namespace Data::Utilities;


StoreInitializationParameters::StoreInitializationParameters(
    __in StoreBehavior storeBehavior,
    __in bool allowReadableSecondary)
    : storeBehavior_(storeBehavior),
    allowReadableSecondary_(allowReadableSecondary),
    version_(SerializationVersion)
{
}

StoreInitializationParameters::~StoreInitializationParameters()
{
}

//reserved for fileproperties.h
NTSTATUS StoreInitializationParameters::Create(
    __in KAllocator & allocator,
    __out StoreInitializationParameters::SPtr & result)
{
    NTSTATUS status;
    SPtr output = _new(STOREINITPARAM_TAG, allocator) StoreInitializationParameters(StoreBehavior::SingleVersion, true);

    if (!output)
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

NTSTATUS StoreInitializationParameters::Create(
    __in KAllocator & allocator,
    __in StoreBehavior storeBehavior,
    __in bool allowReadableSecondary,
    __out StoreInitializationParameters::SPtr & result)
{
    NTSTATUS status;
    SPtr output = _new(STOREINITPARAM_TAG, allocator) StoreInitializationParameters(storeBehavior, allowReadableSecondary);

    if (!output)
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


void StoreInitializationParameters::Write(__in BinaryWriter & writer)
{
    // 'behavior' - byte
    writer.Write(storeBehaviorPropertyName_);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(byte)));

    writer.Write((byte)storeBehavior_);

    // 'readablesecondary' - bool
    writer.Write(allowReadableSecondaryPropertyName_);
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(bool)));
    writer.Write(allowReadableSecondary_);
}

void StoreInitializationParameters::ReadProperty(
    __in BinaryReader & reader,
    __in KStringView const & property,
    __in ULONG32 valueSize)
{
    if (property == storeBehaviorPropertyName_)
    {
        byte data = 0;
        reader.Read(data);
        storeBehavior_ = (StoreBehavior)data;
    }
    else if (property == allowReadableSecondaryPropertyName_)
    {
        reader.Read(allowReadableSecondary_);
    }
    else
    {
        FileProperties::ReadProperty(reader, property, valueSize);
    }
}

StoreInitializationParameters::SPtr StoreInitializationParameters::FromOperationData(
    __in KAllocator& allocator,
    __in OperationData const & initializationContext)
{
    KBuffer::SPtr bufferSPtr = const_cast<KBuffer *>(initializationContext[0].RawPtr());
    BinaryReader reader(*bufferSPtr, allocator);

    // Read and check the version first.  This is for future proofing - if the format dramatically changes,
    // we can change the serialization version and modify the serialization/deserialization code.
    // For now, we verify the version matches exactly the expected version.
    ULONG32 version = 0;
    reader.Read(version);

    if (version != SerializationVersion)
    {
        //todo: throw InvalidDataException
        throw ktl::Exception(STATUS_INTERNAL_DB_CORRUPTION);
    }

    // Currently, the rest of the properties are serialized as [string, value] pairs (see ReadProperty and Write methods).
    BlockHandle::SPtr handleSPtr = nullptr;
    NTSTATUS status = BlockHandle::Create(sizeof(int), bufferSPtr->QuerySize() - sizeof(int), allocator, handleSPtr);
    Diagnostics::Validate(status);

    StoreInitializationParameters::SPtr initializationParametersSPtr = FileProperties::Read<StoreInitializationParameters>(reader, *handleSPtr, allocator);

    initializationParametersSPtr->Version = version;
    return initializationParametersSPtr;
}

OperationData::SPtr StoreInitializationParameters::ToOperationData(__in KAllocator& allocator)
{
    OperationData::SPtr initParameters = OperationData::Create(allocator);

    BinaryWriter writer(allocator);
    // Always write the version first, so we can change the serialization code if necessary.
    writer.Write(version_);

    // Write the remainder of the properties as [string, value] pairs.
    Write(writer);

    KBuffer::SPtr bufferSPtr = writer.GetBuffer(0);
    initParameters->Append(*bufferSPtr);
    return initParameters;
}
