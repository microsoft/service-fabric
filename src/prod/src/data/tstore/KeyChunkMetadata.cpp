// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#define KEYCHUNKMETADATA_TAG 'dmCK'

using namespace Data::TStore;
using namespace Data::Utilities;

const ULONG32 KeyChunkMetadata::Size;

KeyChunkMetadata::KeyChunkMetadata(__in ULONG32 blockSize)
    :blockSize_(blockSize)
{
}

KeyChunkMetadata KeyChunkMetadata::Read(__in BinaryReader& reader)
{
    ByteAlignedReaderWriterHelper::AssertIfNotAligned(reader.Position);

    ULONG32 currentBlockSize = 0;
    reader.Read(currentBlockSize);

    ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);
    ByteAlignedReaderWriterHelper::AssertIfNotAligned(reader.Position);

    KeyChunkMetadata keyChunkMetadata(currentBlockSize);
    return keyChunkMetadata;

}

void KeyChunkMetadata::Write(__in BinaryWriter& writer)
{
    ByteAlignedReaderWriterHelper::AssertIfNotAligned(writer.Position);
    writer.Write(blockSize_);
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);
    ByteAlignedReaderWriterHelper::AssertIfNotAligned(writer.Position);
}
