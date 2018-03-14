// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace ktl;
using namespace Data::TStore;
using namespace Data::Utilities;

const ULONG32 PropertyChunkMetadata::Size;

PropertyChunkMetadata::PropertyChunkMetadata(__in ULONG32 blockSize)
   :blockSize_(blockSize)
{
}

PropertyChunkMetadata::~PropertyChunkMetadata()
{
}

PropertyChunkMetadata PropertyChunkMetadata::Read(__in BinaryReader& reader)
{
   ByteAlignedReaderWriterHelper::ThrowIfNotAligned(reader.Position);

   ULONG32 currentBlockSize = 0;
   reader.Read(currentBlockSize);
   ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);

   PropertyChunkMetadata propertyChunkMetadata(currentBlockSize);

   return propertyChunkMetadata;
}

void PropertyChunkMetadata::Write(__in BinaryWriter& writer)
{
   ByteAlignedReaderWriterHelper::AssertIfNotAligned(writer.Position);

   writer.Write(blockSize_);
   ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);

   ByteAlignedReaderWriterHelper::AssertIfNotAligned(writer.Position);
}
