// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define KEYCHECKPOINTFILEPROPERTIES_TAG 'pfCK'

using namespace Data::TStore;
using namespace Data::Utilities;

KeyCheckpointFileProperties::KeyCheckpointFileProperties()
    :keysHandleSPtr_(nullptr),
    keyCount_(0),
    fileId_(0)
{
}

KeyCheckpointFileProperties::~KeyCheckpointFileProperties()
{
}

NTSTATUS KeyCheckpointFileProperties::Create(
    __in KAllocator& allocator,
    __out KeyCheckpointFileProperties::SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(KEYCHECKPOINTFILEPROPERTIES_TAG, allocator) KeyCheckpointFileProperties();

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

void KeyCheckpointFileProperties::Write(__in BinaryWriter& writer)
{
    ByteAlignedReaderWriterHelper::AssertIfNotAligned(writer.Position);

    // 'KeysHandle' - BlockHandle
    writer.Write(static_cast<LONG32>(PropertyId::KeysHandleProp));
    VarInt::Write(writer, BlockHandle::SerializedSize());
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);
    keysHandleSPtr_->Write(writer);

    // 'KeyCount' - long
    writer.Write(static_cast<LONG32>(PropertyId::KeyCountProp));
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG64)));
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);
    writer.Write(keyCount_);

    // 'FileId' - int
    writer.Write(static_cast<LONG32>(PropertyId::FileIdProp));
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG32)));
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);
    writer.Write(fileId_);
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);

    ByteAlignedReaderWriterHelper::AssertIfNotAligned(writer.Position);
}

void KeyCheckpointFileProperties::ReadProperty(
    __in BinaryReader& reader,
    __in ULONG32 property,
    __in ULONG32 valueSize)
{
    ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);
    switch (static_cast<PropertyId>(property))
    {
    case PropertyId::KeysHandleProp:
        keysHandleSPtr_ = BlockHandle::Read(reader, GetThisAllocator());
        break;

    case PropertyId::KeyCountProp:
        reader.Read(keyCount_);
        break;

    case PropertyId::FileIdProp:
        reader.Read(fileId_);
        ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);
        break;

    default:
        FilePropertySection::ReadProperty(reader, property, valueSize);
        break;
    }

    ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);
}
