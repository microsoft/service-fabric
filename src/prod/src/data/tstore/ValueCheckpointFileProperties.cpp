// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define VALUECHECKPOINTFILEPROPERTIES_TAG 'pfCV'

using namespace Data::TStore;
using namespace Data::Utilities;

ValueCheckpointFileProperties::ValueCheckpointFileProperties()
    :valuesHandleSPtr_(nullptr),
    valueCount_(0),
    fileId_(0)
{
}

ValueCheckpointFileProperties::~ValueCheckpointFileProperties()
{
}

NTSTATUS ValueCheckpointFileProperties::Create(
    __in KAllocator& allocator,
    __out ValueCheckpointFileProperties::SPtr& result)
{
    NTSTATUS status;
    SPtr output = _new(VALUECHECKPOINTFILEPROPERTIES_TAG, allocator) ValueCheckpointFileProperties();

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

void ValueCheckpointFileProperties::Write(__in BinaryWriter& writer)
{
    //TODO: fix size.
    //KSharedPtr<BinaryWriter> writer = &bw;
    ByteAlignedReaderWriterHelper::AssertIfNotAligned(writer.Position);

    // 'ValuesHandle' - BlockHandle
    writer.Write(static_cast<ULONG32>(PropertyId::ValuesHandleProp));
    VarInt::Write(writer, BlockHandle::SerializedSize());
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);
    valuesHandleSPtr_->Write(writer);

    // 'ValueCount' - long
    writer.Write(static_cast<ULONG32>(PropertyId::ValueCountProp));
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG64)));
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);
    writer.Write(valueCount_);

    // 'FileId' - int
    writer.Write(static_cast<ULONG32>(PropertyId::FileIdProp));
    VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG32)));
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);
    writer.Write(fileId_);
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);

    ByteAlignedReaderWriterHelper::AssertIfNotAligned(writer.Position);
}

void ValueCheckpointFileProperties::ReadProperty(
    __in BinaryReader& reader,
    __in ULONG32 property,
    __in ULONG32 valueSize)
{
    ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);
    switch (static_cast<PropertyId>(property))
    {
    case PropertyId::ValuesHandleProp:
        valuesHandleSPtr_ = BlockHandle::Read(reader, GetThisAllocator());
        break;

    case PropertyId::ValueCountProp:
        reader.Read(valueCount_);
        break;

    case PropertyId::FileIdProp:
        reader.Read(fileId_);
        ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);
        break;

    default:
        FilePropertySection::ReadProperty(reader, property, valueSize);
        ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);
        break;
    }

    ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);   
}
