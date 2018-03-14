// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define METADATAMANAGERFP_TAG 'pfDM'

using namespace Data::TStore;
using namespace Data::Utilities;

MetadataManagerFileProperties::MetadataManagerFileProperties()
   : fileCount_(0),
   checkpointLSN_(-1),
   metadataHandleSPtr_(nullptr)
{
}

MetadataManagerFileProperties::~MetadataManagerFileProperties()
{
}

bool MetadataManagerFileProperties::Test_IgnoreCheckpointLSN = false;

NTSTATUS MetadataManagerFileProperties::Create(
   __in KAllocator& allocator,
   __out MetadataManagerFileProperties::SPtr& result)
{
   result = _new(METADATAMANAGERFP_TAG, allocator) MetadataManagerFileProperties();

   if (result == nullptr)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   if (!NT_SUCCESS(result->Status()))
   {
      // Null Result while fetching failure status with no extra AddRefs or Releases
      return (SPtr(Ktl::Move(result)))->Status();
   }

   return STATUS_SUCCESS;
}

void MetadataManagerFileProperties::Write(__in BinaryWriter& writer)
{
   KInvariant(metadataHandleSPtr_ != nullptr);
   writer.Write(static_cast<ULONG32>(PropertyId::MetadataHandle));
   VarInt::Write(writer, static_cast<ULONG32>(BlockHandle::SerializedSize()));
   metadataHandleSPtr_->Write(writer);

   // FileCount
   writer.Write(static_cast<ULONG32>(PropertyId::DataFileCount));
   VarInt::Write(writer, static_cast<ULONG32>(sizeof(ULONG32)));
   writer.Write(static_cast<ULONG32>(fileCount_));

   if (Test_IgnoreCheckpointLSN)
   {
       return;
   }

   // 'CheckpointLSN' - LONG64
   writer.Write(static_cast<ULONG32>(PropertyId::CheckpointLSN));
   VarInt::Write(writer, static_cast<ULONG32>(sizeof(LONG64)));
   writer.Write(checkpointLSN_);
}

void MetadataManagerFileProperties::ReadProperty(
   __in BinaryReader& reader,
   __in ULONG32 property,
   __in ULONG32 valueSize)
{
   switch (static_cast<PropertyId::Enum>(property))
   {
   case PropertyId::MetadataHandle:
   {
      BlockHandle::SPtr metadataHandleSPtr = BlockHandle::Read(reader, GetThisAllocator());
      MetadataHandleSPtr = *metadataHandleSPtr;
      break;
   }

   case PropertyId::DataFileCount:
   {
      ULONG32 fileCount = 0;
      reader.Read(fileCount);
      FileCount = fileCount;
      break;
   }

   case PropertyId::CheckpointLSN:
   {
      LONG64 checkpointLSN = 0;
      reader.Read(checkpointLSN);
      CheckpointLSN = checkpointLSN;
      break;
   }

   default:
      FilePropertySection::ReadProperty(reader, property, valueSize);
      break;
   }
}
