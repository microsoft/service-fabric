// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      class FileMetadata : 
          public KObject<FileMetadata>,
          public KShared<FileMetadata>
      {
         K_FORCE_SHARED(FileMetadata)

      public:

         static NTSTATUS
            Create(
               __in ULONG32 fileId,
               __in KString& fileName,
               __in ULONG64 totalNumberOfEntries,
               __in ULONG64 numberOfValidEntries,
               __in ULONG64 logicalTimeStamp,
               __in ULONG64 numberOfDeletedEntries,
               __in bool canBeDeleted,
               __in KAllocator& allocator,
               __in StoreTraceComponent & traceComponent,
               __out FileMetadata::SPtr& result);

         __declspec(property(get = get_NumberOfValidEntries, put = set_NumberOfValidEntries)) LONG64 NumberOfValidEntries;
         LONG64 get_NumberOfValidEntries() const
         {
            return numberOfValidEntries_;
         }

         void set_NumberOfValidEntries(LONG64 value)
         {
            numberOfValidEntries_ = value;
         }

         __declspec(property(get = get_TotalNumberOfEntries, put = set_TotalNumberOfEntries)) LONG64 TotalNumberOfEntries;
         LONG64 get_TotalNumberOfEntries() const
         {
            return totalNumberOfEntries_;
         }

         void set_TotalNumberOfEntries(LONG64 value)
         {
            totalNumberOfEntries_ = value;
         }

         __declspec(property(get = get_NumberOfDeletedEntries, put = set_NumberOfDeletedEntries)) LONG64 NumberOfDeletedEntries;
         LONG64 get_NumberOfDeletedEntries() const
         {
            return numberOfDeletedEntries_;
         }

         void set_NumberOfDeletedEntries(LONG64 value)
         {
            numberOfDeletedEntries_ = value;
         }

         __declspec(property(get = get_FileId, put = set_FileId)) ULONG32 FileId;
         ULONG32 get_FileId() const
         {
            return fileId_;
         }

         void set_FileId(ULONG32 value)
         {
            fileId_ = value;
         }

         __declspec(property(get = get_ReferenceCount)) LONG64 ReferenceCount;
         LONG64 get_ReferenceCount() const
         {
             return referenceCount_;
         }

         __declspec(property(get = get_CanBeDeleted, put = set_CanBeDeleted)) bool CanBeDeleted;
         bool get_CanBeDeleted() const
         {
            return canBeDeleted_;
         }

         void set_CanBeDeleted(bool value)
         {
            canBeDeleted_ = value;
         }

         __declspec(property(get = get_LogicalTimeStamp, put = set_LogicalTimeStamp)) LONG64 LogicalTimeStamp;
         LONG64 get_LogicalTimeStamp() const
         {
            return logicalTimeStamp_;
         }

         void set_LogicalTimeStamp(LONG64 value)
         {
            logicalTimeStamp_ = value;
         }

         __declspec(property(get = get_FileName, put = set_FileName)) KString::SPtr FileName;
         KString::SPtr get_FileName() const
         {
            return fileName_;
         }

         void set_FileName(KString& value)
         {
            fileName_ = &value;
         }

         _declspec(property(get = get_CheckpointFile, put = set_CheckpointFile)) CheckpointFile::SPtr CheckpointFileSPtr;
         CheckpointFile::SPtr get_CheckpointFile() const
         {
             return checkpointFileSPtr_;
         }

         void set_CheckpointFile(__in CheckpointFile& value)
         {
             checkpointFileSPtr_ = &value;
         }

         ktl::Awaitable<void> CloseAsync();
         ktl::Awaitable<ULONG64> GetFileSizeAsync();
         void Write(__in BinaryWriter& writer);

         static FileMetadata::SPtr Read(
             __in BinaryReader& reader,
             __in KAllocator& allocator,
             __in StoreTraceComponent & traceComponent);

         void DecrementValidEntries();

         static ULONG HashFunction(__in const FileMetadata::SPtr & key);

         bool TryAddReference();
         void AddReference();
         ktl::Awaitable<void> ReleaseReferenceAsync();

      private:
         ULONG32 fileId_;
         KString::SPtr fileName_;
         ULONG64 totalNumberOfEntries_;
         LONG64 numberOfValidEntries_;
         ULONG64 logicalTimeStamp_;
         ULONG64 numberOfDeletedEntries_;
         bool canBeDeleted_;
         bool isClosed_;
         LONG64 referenceCount_;
         Data::TStore::CheckpointFile::SPtr checkpointFileSPtr_;

         StoreTraceComponent::SPtr traceComponent_;

         FileMetadata(
            __in ULONG32 fileId,
            __in KString& fileName,
            __in ULONG64 totalNumberOfEntries,
            __in LONG64 numberOfValidEntries,
            __in ULONG64 logicalTimeStamp,
            __in ULONG64 numberOfDeletedEntries,
            __in bool canBeDeleted,
            __in StoreTraceComponent & traceComponent);
      };
   }
}

