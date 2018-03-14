// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      class MetadataManagerFileProperties : public FilePropertySection
      {
         K_FORCE_SHARED(MetadataManagerFileProperties)
      public:
          //todo: this supposed to be non-static so every instance can decide how it tests it.
         static bool Test_IgnoreCheckpointLSN;

         static NTSTATUS
            Create(
               __in KAllocator& Allocator,
               __out MetadataManagerFileProperties::SPtr& Result);

         __declspec(property(get = get_FileCount, put = set_FileCount)) ULONG32 FileCount;
         ULONG32 get_FileCount() const
         {
            return fileCount_;
         }

         void set_FileCount(ULONG32 value)
         {
            fileCount_ = value;
         }

         __declspec(property(get = get_MetadataHandleSPtr, put = set_MetadataHandleSPtr)) BlockHandle::SPtr MetadataHandleSPtr;
         BlockHandle::SPtr get_MetadataHandleSPtr() const
         {
            return metadataHandleSPtr_;
         }

         void set_MetadataHandleSPtr(__in BlockHandle& value)
         {
            metadataHandleSPtr_ = &value;
         }

         __declspec(property(get = get_CheckpointLSN, put = set_CheckpointLSN)) LONG64 CheckpointLSN;
         LONG64 get_CheckpointLSN() const
         {
            return checkpointLSN_;
         }

         void set_CheckpointLSN(__in LONG64 value)
         {
            checkpointLSN_ = value;
         }

         void Write(__in BinaryWriter& writer) override;
         void ReadProperty(
            __in BinaryReader& reader,
            __in ULONG32 property,
            __in ULONG32 valueSize) override;

      private:
         __in ULONG32 fileCount_;
         __in LONG64 checkpointLSN_;
         __in BlockHandle::SPtr metadataHandleSPtr_;

      };
   }
}
