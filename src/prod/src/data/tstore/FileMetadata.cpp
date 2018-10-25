// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#define FILEMETADATA_Tag 'tdMF'

using namespace Data::TStore;
using namespace Data::Utilities;

FileMetadata::FileMetadata(
   __in ULONG32 fileId,
   __in KString& fileName,
   __in ULONG64 totalNumberOfEntries,
   __in LONG64 numberOfValidEntries,
   __in ULONG64 logicalTimeStamp,
   __in ULONG64 numberOfDeletedEntries,
   __in bool canBeDeleted,
    __in StoreTraceComponent & traceComponent)
   : fileId_(fileId),
   fileName_(&fileName),
   totalNumberOfEntries_(totalNumberOfEntries),
   numberOfValidEntries_(numberOfValidEntries),
   logicalTimeStamp_(logicalTimeStamp),
   numberOfDeletedEntries_(numberOfDeletedEntries),
   canBeDeleted_(canBeDeleted),
   isClosed_(false),
   referenceCount_(1), // Start with reference count 1 to account for adding ref on creation time.
   traceComponent_(&traceComponent)
{
}

FileMetadata:: ~FileMetadata()
{
   // todo: Might need to delete checkpoint files, based on flasg.
}

NTSTATUS FileMetadata::Create(
   __in ULONG32 fileId,
   __in KString& fileName,
   __in ULONG64 totalNumberOfEntries,
   __in ULONG64 numberOfValidEntries,
   __in ULONG64 logicalTimeStamp,
   __in ULONG64 numberOfDeletedEntries,
   __in bool canBeDeleted,
   __in KAllocator& allocator,
   __in StoreTraceComponent & traceComponent,
   __out FileMetadata::SPtr& result)
{
   result = _new(FILEMETADATA_Tag, allocator) FileMetadata(
      fileId,
      fileName,
      totalNumberOfEntries,
      numberOfValidEntries,
      logicalTimeStamp,
      numberOfDeletedEntries,
      canBeDeleted,
      traceComponent);

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

ktl::Awaitable<ULONG64> FileMetadata::GetFileSizeAsync()
{
    //todo: replace checkpointfile internal getfilesize method with common file utility method.
    STORE_ASSERT(checkpointFileSPtr_ != nullptr, "FileMetadata::checkpointFileSPtr_ is null");
    return checkpointFileSPtr_->GetTotalFileSizeAsync(GetThisAllocator());
}

void FileMetadata::Write(__in BinaryWriter& writer)
{
    // Asert is aligned
    writer.Write(totalNumberOfEntries_);
    writer.Write(numberOfValidEntries_);
    writer.Write(numberOfDeletedEntries_);
    writer.Write(logicalTimeStamp_);

    // Padding needed for the fields below
    writer.Write(fileId_);
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);

    writer.Write(*fileName_);
    ByteAlignedReaderWriterHelper::WritePaddingUntilAligned(writer);
}

//
// Serialization is 8 byte aligned. 
//
// Padding is needd for fileId, fileName and canBeDleted flags.
FileMetadata::SPtr FileMetadata::Read(
    __in BinaryReader& reader,
    __in KAllocator& allocator,
    __in StoreTraceComponent & traceComponent)
{
    ULONG64 totalNumberOfEntries = 0;
    reader.Read(totalNumberOfEntries);

    ULONG64 numberOfValidEntries = 0;
    reader.Read(numberOfValidEntries);

    ULONG64 numberOfDeletedEntries = 0;
    reader.Read(numberOfDeletedEntries);

    ULONG64 logicalTimeStamp = 0;
    reader.Read(logicalTimeStamp);

    ULONG32 fileId = 0;
    reader.Read(fileId);
    ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);

    KString::SPtr fileName = nullptr;
    reader.Read(fileName);
    ASSERT_IFNOT(fileName != nullptr, "{0}: filename should not be null", traceComponent.AssertTag);
    ByteAlignedReaderWriterHelper::ReadPaddingUntilAligned(reader);

    FileMetadata::SPtr fileMetdataSPtr = nullptr;
    NTSTATUS status = FileMetadata::Create(
        fileId, 
        *fileName, 
        totalNumberOfEntries, 
        numberOfValidEntries, 
        logicalTimeStamp, 
        numberOfDeletedEntries, 
        false, 
        allocator, 
        traceComponent, 
        fileMetdataSPtr);
    Diagnostics::Validate(status);
    return fileMetdataSPtr;
}

void FileMetadata::DecrementValidEntries()
{
   // todo
    InterlockedDecrement64(&numberOfValidEntries_);
}


ULONG FileMetadata::HashFunction(const FileMetadata::SPtr & key)
{
    return key->FileId;
}

bool FileMetadata::TryAddReference()
{
    LONG64 refCount = 0;

    // Spin instead of locking since contention is low.
    do
    {
        //todo: implement Volatile.
        //refCount = Volatile.Read(ref this.referenceCount);
        if (refCount == 0)
            return false;
    } while (InterlockedCompareExchange64(&referenceCount_, refCount + 1, refCount) != refCount);

    return true;
}

void FileMetadata::AddReference()
{
    LONG64 refCount = InterlockedIncrement64(&referenceCount_);

    STORE_ASSERT(refCount >= 2, "Unbalanced AddRef-ReleaseRef detected! RefCount: {1}", refCount);
}

ktl::Awaitable<void> FileMetadata::ReleaseReferenceAsync()
{
    LONG64 refCount = InterlockedDecrement64(&referenceCount_);
    STORE_ASSERT(refCount >= 0, "Unbalanced AddRef-ReleaseRef detected! {1}", refCount);

    if (refCount == 0)
    {
        co_await CloseAsync();
    }
}

ktl::Awaitable<void> FileMetadata::CloseAsync() 
{
    try
    {
        if (isClosed_)
        {
            co_return;
        }

        if (checkpointFileSPtr_ != nullptr)
        {
            KString::SPtr keyFileNameSPtr = checkpointFileSPtr_->KeyCheckpointFileNameSPtr;
            KString::SPtr valueFileNameSPtr = checkpointFileSPtr_->ValueCheckpointFileNameSPtr;

            co_await checkpointFileSPtr_->CloseAsync();

            if (canBeDeleted_)
            {
                KStringView keyFileName(*keyFileNameSPtr);
                KStringView valueFileName(*valueFileNameSPtr);

                StoreEventSource::Events->FileMetadataDeleteKeyValue(
                    traceComponent_->PartitionId,
                    traceComponent_->TraceTag,
                    ToStringLiteral(keyFileName),
                    ToStringLiteral(valueFileName));

                auto errorCode = Common::File::Delete2(keyFileName.operator LPCWSTR());
                if (errorCode.IsSuccess() == false)
                {
                    throw ktl::Exception(errorCode.ToHResult());
                }

                errorCode = Common::File::Delete2(valueFileName.operator LPCWSTR());
                if (errorCode.IsSuccess() == false)
                {
                    throw ktl::Exception(errorCode.ToHResult());
                }
            }

            isClosed_ = true;
        }
    }
    catch (ktl::Exception const & exception)
    {
        StoreEventSource::Events->FileMetadataCloseException(
            traceComponent_->PartitionId,
            traceComponent_->TraceTag,
            static_cast<LONG64>(exception.GetStatus()));
        throw;
    }
}
