// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TStoreTests
{
   using namespace ktl;
   using namespace Data::TStore;
   using namespace Data::TestCommon;

   class DiskMetadatTest
   {
   public:
      DiskMetadatTest()
      {
         NTSTATUS status = KtlSystem::Initialize(FALSE, &ktlSystem_);
         CODING_ERROR_ASSERT(NT_SUCCESS(status));
         ktlSystem_->SetStrictAllocationChecks(TRUE);
      }

      ~DiskMetadatTest()
      {
          ktlSystem_->Shutdown();
      }

      KAllocator& GetAllocator()
      {
         return ktlSystem_->NonPagedAllocator();
      }

      StoreTraceComponent::SPtr CreateTraceComponent()
      {
          KGuid guid;
          guid.CreateNew();
          ::FABRIC_REPLICA_ID replicaId = 1;
          int stateProviderId = 1;

          StoreTraceComponent::SPtr traceComponent = nullptr;
          NTSTATUS status = StoreTraceComponent::Create(guid, replicaId, stateProviderId, GetAllocator(), traceComponent);
          CODING_ERROR_ASSERT(NT_SUCCESS(status));

          return traceComponent;
      }

      ktl::io::KFileStream::SPtr GetFileStream()
      {
         CODING_ERROR_ASSERT(fakeStreamSPtr_ != nullptr);
         return fakeStreamSPtr_;
      }

      KString::SPtr CreateFileName(__in KString& name)
      {
          KString::SPtr temp = nullptr;
          return CreateFileName(KWString(GetAllocator(), name), temp);
      }

      KString::SPtr CreateFileName(__in KWString const & name, __out KString::SPtr& currentFilePathSPtr)
      {
         WCHAR currentDirectoryPathCharArray[MAX_PATH] = {L'O'};
         GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);
         PathAppendW(currentDirectoryPathCharArray, name);

         #if !defined(PLATFORM_UNIX)
         KWString fileName(GetAllocator(), L"\\??\\");
         #else
         KWString fileName(GetAllocator(), L"");
         #endif
         VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));

         KWString currentFilePath(GetAllocator(), currentDirectoryPathCharArray);
         VERIFY_IS_TRUE(NT_SUCCESS(currentFilePath.Status()));

         auto status = KString::Create(currentFilePathSPtr, GetAllocator(), static_cast<LPCWSTR>(currentFilePath));
         CODING_ERROR_ASSERT(NT_SUCCESS(status));

         fileName += currentFilePath;

         KString::SPtr result;
         status = KString::Create(result, GetAllocator(), static_cast<LPCWSTR>(fileName));
         CODING_ERROR_ASSERT(NT_SUCCESS(status));
         return result;
      }

      void CreateFileStream(__in KWString& name)
      {
         KWString fileName(GetAllocator(), *CreateFileName(name, currentFilePath_));
         NTSTATUS status = SyncAwait(FileFormatTestHelper::CreateBlockFile(fileName, GetAllocator(), CancellationToken::None, fakeFileSPtr, fakeStreamSPtr_));
         VERIFY_IS_TRUE(NT_SUCCESS(status));

         status = SyncAwait(fakeStreamSPtr_->OpenAsync(*fakeFileSPtr));
         VERIFY_IS_TRUE(NT_SUCCESS(status));

      }

      void CloseFileStream(bool removeFile = true)
      {
          if (fakeStreamSPtr_ != nullptr && fakeStreamSPtr_->IsOpen())
          {
              SyncAwait(fakeStreamSPtr_->CloseAsync());
              fakeStreamSPtr_ = nullptr;
          }

          if (fakeFileSPtr != nullptr)
          {
              fakeFileSPtr->Close();
              fakeFileSPtr = nullptr;
          }

          if (currentFilePath_ != nullptr && removeFile)
          {
              wstring pathToRemove = currentFilePath_->operator LPCWSTR();
              Common::File::Delete(pathToRemove);
              currentFilePath_ = nullptr;
          }
      }

      void Validate(MetadataTable& expectedMetadataTable, MetadataTable& actualMetadataTable)
      {
         CODING_ERROR_ASSERT(expectedMetadataTable.CheckpointLSN == actualMetadataTable.CheckpointLSN);
         Validate(*expectedMetadataTable.Table, *actualMetadataTable.Table);
      }

      void FlipBitAndVerifyCorruptionAsync(__in ULONG64 byteIndex, ktl::io::KFileStream& filestream)
      {
          bool hasEx = false;
          // Flip a random bit.
          filestream.Position = byteIndex;
          Common::Random random = Common::Random();

          byte bitToFlip = (byte)(0x1 << random.Next(8));
          byte orginalByte = 0;

          KBuffer::SPtr originalDataSPtr;
          NTSTATUS status = KBuffer::Create(1, originalDataSPtr, GetAllocator());
          CODING_ERROR_ASSERT(NT_SUCCESS(status));
          ULONG byteLen = 1;
          ULONG byteRead = 0;
          status = SyncAwait(filestream.ReadAsync(*originalDataSPtr, byteRead, 0, byteLen));
          CODING_ERROR_ASSERT(NT_SUCCESS(status));
          CODING_ERROR_ASSERT(byteRead == byteLen);
          BinaryReader br(*originalDataSPtr, GetAllocator());
          br.Read(orginalByte);

          byte newByte = orginalByte ^ bitToFlip;

          BinaryWriter bw(GetAllocator());
          bw.Write(newByte);

          filestream.Position = byteIndex;
          SyncAwait(filestream.WriteAsync(*bw.GetBuffer(0, 1)));

          // Verify we catch the corruption when reading the file.
          MetadataTable::SPtr metadataTableSPtr = nullptr;
          status = MetadataTable::Create(GetAllocator(), metadataTableSPtr);
          CODING_ERROR_ASSERT(NT_SUCCESS(status));

          filestream.Position = 0;
          try
          {
              SyncAwait(MetadataManager::OpenAsync(*metadataTableSPtr, filestream, filestream.Length, GetAllocator(), *CreateTraceComponent()));
          }
          catch (...)
          {
              hasEx = true;
              // todo: InvalidDataException 
          }

          CODING_ERROR_ASSERT(hasEx);
          SyncAwait(metadataTableSPtr->CloseAsync());
      }

   private:
      KtlSystem* ktlSystem_;
      KBlockFile::SPtr fakeFileSPtr = nullptr;
      ktl::io::KFileStream::SPtr fakeStreamSPtr_ = nullptr;
      KString::SPtr currentFilePath_;

      void Validate(
         __in IDictionary<ULONG32, FileMetadata::SPtr>& expectedTable,
         __in IDictionary<ULONG32, FileMetadata::SPtr>& actualTable)
      {
         CODING_ERROR_ASSERT(expectedTable.Count == actualTable.Count);

         KSharedPtr<IEnumerator<KeyValuePair<ULONG32, FileMetadata::SPtr>>> enumerator = actualTable.GetEnumerator();

         while (enumerator->MoveNext())
         {
            KeyValuePair<ULONG32, FileMetadata::SPtr> entry = enumerator->Current();
            ULONG32 key = entry.Key;
            FileMetadata::SPtr fileMetadata1SPtr = entry.Value;

            FileMetadata::SPtr fileMetadata2SPtr = nullptr;
            bool result = expectedTable.TryGetValue(key, fileMetadata2SPtr);
            CODING_ERROR_ASSERT(result == true);

            CODING_ERROR_ASSERT(fileMetadata1SPtr->FileId == fileMetadata2SPtr->FileId);
            CODING_ERROR_ASSERT(fileMetadata1SPtr->FileName->Compare(*fileMetadata2SPtr->FileName) == 0);
            CODING_ERROR_ASSERT(fileMetadata1SPtr->TotalNumberOfEntries == fileMetadata2SPtr->TotalNumberOfEntries);
            CODING_ERROR_ASSERT(fileMetadata1SPtr->NumberOfValidEntries == fileMetadata2SPtr->NumberOfValidEntries);
            CODING_ERROR_ASSERT(fileMetadata1SPtr->CanBeDeleted == fileMetadata2SPtr->CanBeDeleted);
         }
      }
   };

   BOOST_FIXTURE_TEST_SUITE(DiskMetadatTestSuite, DiskMetadatTest)
       
   BOOST_AUTO_TEST_CASE(WriteRead_MultipleEntries_ShouldSucceed)
   {
      KAllocator& allocator = GetAllocator();
      KWString fileStreamName(allocator, L"WriteRead_MultipleEntries_ShouldSucceed.txt");
      CreateFileStream(fileStreamName);

      FileMetadata::SPtr fileMetadata1 = nullptr;
      KString::SPtr fileName1SPtr = nullptr;
      NTSTATUS status = KString::Create(fileName1SPtr, allocator, L"test1");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(2, *fileName1SPtr, 2, 2, 2, 2, false, allocator, *CreateTraceComponent(), fileMetadata1);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      FileMetadata::SPtr fileMetadata2 = nullptr;
      KString::SPtr fileName2SPtr = nullptr;
      status = KString::Create(fileName2SPtr, allocator, L"test2");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(3, *fileName2SPtr, 3, 3, 3, 3, false, allocator, *CreateTraceComponent(), fileMetadata2);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      FileMetadata::SPtr fileMetadata3 = nullptr;
      KString::SPtr fileName3SPtr = nullptr;
      status = KString::Create(fileName3SPtr, allocator, L"test3");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(4, *fileName3SPtr, 4, 4, 4, 4, false, allocator, *CreateTraceComponent(), fileMetadata3);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      FileMetadata::SPtr fileMetadata4 = nullptr;
      KString::SPtr fileName4SPtr = nullptr;
      status = KString::Create(fileName4SPtr, allocator, L"test4");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(5, *fileName4SPtr, 5, 5, 5, 5, false, allocator, *CreateTraceComponent(), fileMetadata4);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      MetadataTable::SPtr metadataTable1SPtr = nullptr;
      status = MetadataTable::Create(allocator, metadataTable1SPtr, 10);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata1->FileId, *fileMetadata1);
      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata2->FileId, *fileMetadata2);
      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata3->FileId, *fileMetadata3);
      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata4->FileId, *fileMetadata4);

      ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
      SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *fileStreamSPtr, allocator));

      // Re-open the metadata.
      MetadataTable::SPtr metadataTable2SPtr = nullptr;
      MetadataTable::Create(allocator, metadataTable2SPtr);

      // Reset position.
      ULONG64 length = fileStreamSPtr->GetPosition();
      fileStreamSPtr->SetPosition(0);
      SyncAwait(MetadataManager::OpenAsync(*metadataTable2SPtr, *fileStreamSPtr, length, allocator, *CreateTraceComponent()));

      Validate(*metadataTable1SPtr, *metadataTable2SPtr);
      SyncAwait(metadataTable1SPtr->CloseAsync());
      SyncAwait(metadataTable2SPtr->CloseAsync());

      CloseFileStream();
   }

   BOOST_AUTO_TEST_CASE(WriteRead_ToMemoryBuffer_MultipleEntries_ShouldSucceed)
   {
      KAllocator& allocator = GetAllocator();

      FileMetadata::SPtr fileMetadata1 = nullptr;
      KString::SPtr fileName1SPtr = nullptr;
      NTSTATUS status = KString::Create(fileName1SPtr, allocator, L"test1");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(2, *fileName1SPtr, 2, 2, 2, 2, false, allocator, *CreateTraceComponent(), fileMetadata1);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      FileMetadata::SPtr fileMetadata2 = nullptr;
      KString::SPtr fileName2SPtr = nullptr;
      status = KString::Create(fileName2SPtr, allocator, L"test2");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(3, *fileName2SPtr, 3, 3, 3, 3, false, allocator, *CreateTraceComponent(), fileMetadata2);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      FileMetadata::SPtr fileMetadata3 = nullptr;
      KString::SPtr fileName3SPtr = nullptr;
      status = KString::Create(fileName3SPtr, allocator, L"test3");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(4, *fileName3SPtr, 4, 4, 4, 4, false, allocator, *CreateTraceComponent(), fileMetadata3);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      FileMetadata::SPtr fileMetadata4 = nullptr;
      KString::SPtr fileName4SPtr = nullptr;
      status = KString::Create(fileName4SPtr, allocator, L"test4");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(5, *fileName4SPtr, 5, 5, 5, 5, false, allocator, *CreateTraceComponent(), fileMetadata4);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      MetadataTable::SPtr metadataTable1SPtr = nullptr;
      status = MetadataTable::Create(allocator, metadataTable1SPtr, 10);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata1->FileId, *fileMetadata1);
      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata2->FileId, *fileMetadata2);
      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata3->FileId, *fileMetadata3);
      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata4->FileId, *fileMetadata4);

      MemoryBuffer::SPtr writeMemoryStream;
      MemoryBuffer::Create(GetAllocator(), writeMemoryStream);
      SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *writeMemoryStream, allocator));

      // Expandable memory stream currently doesn't support Read. Dump the contents into another memory stream and then validate
      MemoryBuffer::SPtr readMemoryStream;
      MemoryBuffer::Create(*writeMemoryStream->GetBuffer(), GetAllocator(), readMemoryStream);

      // Re-open the metadata.
      MetadataTable::SPtr metadataTable2SPtr = nullptr;
      MetadataTable::Create(allocator, metadataTable2SPtr);

      // Reset position.
      ULONG64 length = writeMemoryStream->GetPosition();
      SyncAwait(MetadataManager::OpenAsync(*metadataTable2SPtr, *readMemoryStream, length, allocator, *CreateTraceComponent()));

      Validate(*metadataTable1SPtr, *metadataTable2SPtr);
      SyncAwait(metadataTable1SPtr->CloseAsync());
      SyncAwait(metadataTable2SPtr->CloseAsync());
   }

   BOOST_AUTO_TEST_CASE(WriteRead_OneEntry_ShouldSucceed)
   {
      KAllocator& allocator = GetAllocator();
      KWString fileStreamName(allocator, L"WriteRead_OneEntry_ShouldSucceed.txt");
      CreateFileStream(fileStreamName);

      FileMetadata::SPtr fileMetadata1 = nullptr;
      KString::SPtr fileNameSPtr = nullptr;
      NTSTATUS status = KString::Create(fileNameSPtr, allocator, L"test");
      CODING_ERROR_ASSERT(NT_SUCCESS(status));
      status = FileMetadata::Create(2, *fileNameSPtr, 2, 2, 2, 2, false, allocator, *CreateTraceComponent(), fileMetadata1);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      MetadataTable::SPtr metadataTable1SPtr = nullptr;
      status = MetadataTable::Create(allocator, metadataTable1SPtr);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata1->FileId, *fileMetadata1);

      ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
      SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *fileStreamSPtr, allocator));

      // Re-open the metadata.
      MetadataTable::SPtr metadataTable2SPtr = nullptr;
      MetadataTable::Create(allocator, metadataTable2SPtr);

      // Reset position.
      ULONG64 length = fileStreamSPtr->GetPosition();
      fileStreamSPtr->SetPosition(0);
      SyncAwait(MetadataManager::OpenAsync(*metadataTable2SPtr, *fileStreamSPtr, length, allocator, *CreateTraceComponent()));

      Validate(*metadataTable1SPtr, *metadataTable2SPtr);

      SyncAwait(metadataTable1SPtr->CloseAsync());
      SyncAwait(metadataTable2SPtr->CloseAsync());
      CloseFileStream();
   }

   BOOST_AUTO_TEST_CASE(WriteRead_EmptyMetadataTable_ShouldSucceed)
   {
      KAllocator& allocator = GetAllocator();
      KWString fileStreamName(allocator, L"WriteRead_EmptyMetadataTable_ShouldSucceed.txt");
      CreateFileStream(fileStreamName);
      MetadataTable::SPtr metadataTable1SPtr = nullptr;
      NTSTATUS status = MetadataTable::Create(allocator, metadataTable1SPtr);
      CODING_ERROR_ASSERT(NT_SUCCESS(status));

      ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
      SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *fileStreamSPtr, allocator));

      // Re-open the metadata.
      MetadataTable::SPtr metadataTable2SPtr = nullptr;
      MetadataTable::Create(allocator, metadataTable2SPtr);

      // Reset position.
      ULONG64 length = fileStreamSPtr->GetPosition();
      fileStreamSPtr->SetPosition(0);
      SyncAwait(MetadataManager::OpenAsync(*metadataTable2SPtr, *fileStreamSPtr, length, allocator, *CreateTraceComponent()));

      Validate(*metadataTable1SPtr, *metadataTable2SPtr);

      SyncAwait(metadataTable1SPtr->CloseAsync());
      SyncAwait(metadataTable2SPtr->CloseAsync());
      CloseFileStream();
   }

   BOOST_AUTO_TEST_CASE(WriteReadByFilename_OneEntry_ShouldSucceed)
   {
       KAllocator& allocator = GetAllocator();
       KString::SPtr filenameSPtr;
       auto status = KString::Create(filenameSPtr, GetAllocator(), L"WriteReadByFilename_OneEntry_ShouldSucceed.txt");
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       KString::SPtr filepathSPtr = CreateFileName(*filenameSPtr);

       FileMetadata::SPtr fileMetadata1 = nullptr;
       KString::SPtr fileNameSPtr = nullptr;
       status = KString::Create(fileNameSPtr, allocator, L"test");
       CODING_ERROR_ASSERT(NT_SUCCESS(status));
       status = FileMetadata::Create(2, *fileNameSPtr, 2, 2, 2, 2, false, allocator, *CreateTraceComponent(), fileMetadata1);
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       MetadataTable::SPtr metadataTable1SPtr = nullptr;
       status = MetadataTable::Create(allocator, metadataTable1SPtr);
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata1->FileId, *fileMetadata1);

       SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *filepathSPtr, allocator));
           
       // Re-open the metadata.
       MetadataTable::SPtr metadataTable2SPtr = nullptr;
       MetadataTable::Create(allocator, metadataTable2SPtr);

       // Open filestream
       SyncAwait(MetadataManager::OpenAsync(*metadataTable2SPtr, *filepathSPtr, allocator, *CreateTraceComponent()));

       Validate(*metadataTable1SPtr, *metadataTable2SPtr);

       SyncAwait(metadataTable1SPtr->CloseAsync());
       SyncAwait(metadataTable2SPtr->CloseAsync());

       Common::File::Delete(filepathSPtr->operator LPCWSTR());
   }

   BOOST_AUTO_TEST_CASE(MetadataFile_WriteAndValid_ShouldSucceed)
   {
       KAllocator& allocator = GetAllocator();
       KString::SPtr filenameSPtr;
       auto status = KString::Create(filenameSPtr, GetAllocator(), L"MetadataFile_WriteAndValid_ShouldSucceed.txt");
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       KString::SPtr filepathSPtr = CreateFileName(*filenameSPtr);

       FileMetadata::SPtr fileMetadata1 = nullptr;
       KString::SPtr fileNameSPtr = nullptr;
       status = KString::Create(fileNameSPtr, allocator, L"test");
       CODING_ERROR_ASSERT(NT_SUCCESS(status));
       status = FileMetadata::Create(2, *fileNameSPtr, 2, 2, 2, 2, false, allocator, *CreateTraceComponent(), fileMetadata1);
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       MetadataTable::SPtr metadataTable1SPtr = nullptr;
       status = MetadataTable::Create(allocator, metadataTable1SPtr);
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata1->FileId, *fileMetadata1);

       SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *filepathSPtr, allocator));

       SyncAwait(MetadataManager::ValidateAsync(*filepathSPtr, allocator));

       SyncAwait(metadataTable1SPtr->CloseAsync());

       Common::File::Delete(filepathSPtr->operator LPCWSTR());
   }

   //todo:test disable since takes too long and file is very big. move to stress test and verify this test.
   //BOOST_AUTO_TEST_CASE(MetadataFile_ExtremelyLargeMetadata_MultipleFlushesSucceed)
   //{
   //    const ULONG32 NumberOfMetadata = 1024 * 128;
   //    const ULONG64 CheckpointLSN = 17;

   //    KAllocator& allocator = GetAllocator();
   //    KString::SPtr filenameSPtr;
   //    auto status = KString::Create(filenameSPtr, GetAllocator(), L"MetadataFile_ExtremelyLargeMetadata_MultipleFlushesSucceed.txt");
   //    CODING_ERROR_ASSERT(NT_SUCCESS(status));

   //    KString::SPtr filepathSPtr = CreateFileName(*filenameSPtr);

   //    MetadataTable::SPtr metadataTable1SPtr = nullptr;
   //    status = MetadataTable::Create(allocator, metadataTable1SPtr);
   //    CODING_ERROR_ASSERT(NT_SUCCESS(status));

   //    for (ULONG32 i = 1; i < NumberOfMetadata; i++)
   //    {
   //        KString::SPtr fileNameSPtr = nullptr;
   //        status = KString::Create(fileNameSPtr, allocator, L"test");
   //        CODING_ERROR_ASSERT(NT_SUCCESS(status));

   //        FileMetadata::SPtr fileMetadata1 = nullptr;
   //        status = FileMetadata::Create(i, *fileNameSPtr, i, i, i, i, false, allocator, fileMetadata1);
   //        CODING_ERROR_ASSERT(NT_SUCCESS(status));

   //        MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata1->FileId, *fileMetadata1);
   //    }

   //    SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *filepathSPtr, allocator));

   //    // Re-open the metadata.
   //    MetadataTable::SPtr metadataTable2SPtr = nullptr;
   //    MetadataTable::Create(allocator, metadataTable2SPtr);

   //    // Open filestream
   //    SyncAwait(MetadataManager::OpenAsync(*metadataTable2SPtr, *filepathSPtr, allocator));

   //    Validate(*metadataTable1SPtr, *metadataTable2SPtr);
   //    CloseFileStream();
   //}

   BOOST_AUTO_TEST_CASE(Open_CorruptFile_ThrowsDataCorruptException)
   {
       KAllocator& allocator = GetAllocator();
       KWString fileStreamName(allocator, L"Open_CorruptFile_ThrowsDataCorruptException.txt");
       CreateFileStream(fileStreamName);

       FileMetadata::SPtr fileMetadata1 = nullptr;
       KString::SPtr fileNameSPtr = nullptr;
       NTSTATUS status = KString::Create(fileNameSPtr, allocator, L"test");
       CODING_ERROR_ASSERT(NT_SUCCESS(status));
       status = FileMetadata::Create(2, *fileNameSPtr, 2, 2, 2, 2, false, allocator, *CreateTraceComponent(), fileMetadata1);
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       MetadataTable::SPtr metadataTable1SPtr = nullptr;
       status = MetadataTable::Create(allocator, metadataTable1SPtr);
       CODING_ERROR_ASSERT(NT_SUCCESS(status));

       MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata1->FileId, *fileMetadata1);

       ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
       SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *fileStreamSPtr, allocator));

       // Re-open the metadata.
       MetadataTable::SPtr metadataTable2SPtr = nullptr;
       MetadataTable::Create(allocator, metadataTable2SPtr);

       // Reset position.
       ULONG64 length = fileStreamSPtr->GetPosition();
       fileStreamSPtr->SetPosition(0);
       SyncAwait(MetadataManager::OpenAsync(*metadataTable2SPtr, *fileStreamSPtr, length, allocator, *CreateTraceComponent()));

       Validate(*metadataTable1SPtr, *metadataTable2SPtr);
       //flipt first bit
       FlipBitAndVerifyCorruptionAsync(0, *fileStreamSPtr);
       //restore the filestream.
       CloseFileStream(false);
       CreateFileStream(fileStreamName);

       //flip last bit.
       FlipBitAndVerifyCorruptionAsync(length-1, *GetFileStream());
       //restore the filestream.
       CloseFileStream(false);
       CreateFileStream(fileStreamName);

       //flip random bytes
       Common::Random random = Common::Random();
       for (int i = 0; i < 100; i++)
       {
           ULONG64 byteIndex = static_cast<ULONG64>(random.Next(static_cast<int>(length)));
           FlipBitAndVerifyCorruptionAsync(byteIndex, *GetFileStream());
           //restore the filestream.
           CloseFileStream(false);
           CreateFileStream(fileStreamName);
       }

       SyncAwait(metadataTable1SPtr->CloseAsync());
       SyncAwait(metadataTable2SPtr->CloseAsync());
       CloseFileStream();
   }
   
   //Open_PartialFile_ThrowsInvalidDataException()  not testable till FileStream support resize the stream.
   
   //VerifyFileContents   test is ignored.

   //BOOST_AUTO_TEST_CASE(WriteRead_LargeNumberOfEntries_MultipleFlushes_ShouldSucceed)
   //{
   //   // Increase this number once binary read is fixed to not make a copy of the buffer.
   //   ULONG32 numberOfEntries = 100;
   //   LONG64 checkpointLSN = 17L;

   //   KAllocator& allocator = GetAllocator();
   //   KWString fileStreamName(allocator, L"WriteRead_OneEntry_ShouldSucceed.txt");
   //   CreateFileStream(fileStreamName);

   //   MetadataTable::SPtr metadataTable1SPtr = nullptr;
   //   NTSTATUS status = MetadataTable::Create(allocator, metadataTable1SPtr);
   //   CODING_ERROR_ASSERT(NT_SUCCESS(status));

   //   metadataTable1SPtr->CheckpointLSN = checkpointLSN;

   //   for (ULONG32 i = 0; i < numberOfEntries; i++)
   //   {
   //      FileMetadata::SPtr fileMetadata1 = nullptr;
   //      KString::SPtr fileNameSPtr = nullptr;
   //      status = KString::Create(L"test"+i, allocator, fileNameSPtr);
   //      CODING_ERROR_ASSERT(NT_SUCCESS(status));
   //      NTSTATUS status = FileMetadata::Create(i, *fileNameSPtr, i, i, i, i, false, allocator, fileMetadata1);
   //      CODING_ERROR_ASSERT(NT_SUCCESS(status));

   //      MetadataManager::AddFile(*(metadataTable1SPtr->Table), fileMetadata1->FileId, *fileMetadata1);
   //   }

   //   ktl::io::KFileStream::SPtr fileStreamSPtr = GetFileStream();
   //   SyncAwait(MetadataManager::WriteAsync(*metadataTable1SPtr, *fileStreamSPtr, allocator));

   //   // Re-open the metadata.
   //   MetadataTable::SPtr metadataTable2SPtr = nullptr;
   //   MetadataTable::Create(allocator, metadataTable2SPtr);

   //   // Reset position.
   //   ULONG64 length = fileStreamSPtr->GetPosition();
   //   fileStreamSPtr->SetPosition(0);
   //   SyncAwait(MetadataManager::OpenAsync(*metadataTable2SPtr, *fileStreamSPtr, length, allocator));

   //   Validate(*metadataTable1SPtr, *metadataTable2SPtr);
   //}

   // todo: Pending tests.

   ///// <summary>
   ///// Tests the MetadataManager's ability to detect data corruption due to partial file being written.
   ///// </summary>
   //[TestMethod]
   //public async Task Open_PartialFile_ThrowsInvalidDataException()
   //{
   //   Random random = GetRandom();
   //   MetadataTable metadataTable = CreateRandomMetadataTable(random);
   //   await MetadataManager.WriteAsync(metadataTable, TestPath).ConfigureAwait(false);

   //   // Remove the last bytes from the file, one by one, and verify we catch the corruption.
   //   long metadataFileSize = FabricFile.GetSize(TestPath);
   //   Console.WriteLine("FileSize: {0}", metadataFileSize);
   //   for (long removeByteCount = 1; removeByteCount < 128; removeByteCount++)
   //   {
   //      metadataFileSize--;
   //      await this.shrinkFileAndVerifyCorruptionAsync(
   //         metadataFileSize,
   //         string.Format(
   //            "MetadataManager file should catch data corruption if {0} bytes are removed from the end.",
   //            removeByteCount)).ConfigureAwait(false);
   //   }

   //   // Remove a random number of remaining bytes until the file size is 0.
   //   while (metadataFileSize > 0)
   //   {
   //      long bytesToRemove = random.Next(1, checked((int)metadataFileSize));
   //      metadataFileSize -= bytesToRemove;

   //      await this.shrinkFileAndVerifyCorruptionAsync(
   //         metadataFileSize,
   //         "MetadataManager file should catch data corruption if bytes are removed from the end.").ConfigureAwait(false);
   //   }
   //}

   ///// <summary>
   ///// Tests the MetadataManager's ability to detect data corruption due to random bit flips.
   ///// </summary>
   //[TestMethod]
   //public async Task Open_CorruptFile_ThrowsInvalidDataException()
   //{
   //   Random random = GetRandom();

   //   MetadataTable metadataTable = CreateRandomMetadataTable(random);
   //   await MetadataManager.WriteAsync(metadataTable, TestPath).ConfigureAwait(false);

   //   long metadataFileSize = FabricFile.GetSize(TestPath);
   //   Assert.IsTrue(metadataFileSize > 0, "MetadataManager file should have non-zero size.");

   //   // Flip the first and last bytes in the MetadataManager file.
   //   await this.flipBitAndVerifyCorruptionAsync(
   //      random,
   //      0,
   //      "MetadataManager file should catch data corruption if the first byte is flipped.").ConfigureAwait(false);
   //   await this.flipBitAndVerifyCorruptionAsync(
   //      random,
   //      (int)(metadataFileSize - 1),
   //      "MetadataManager file should catch data corruption if the last byte is flipped.").ConfigureAwait(false);

   //   // Flip random bytes in the stream.
   //   for (int i = 0; i < 100; i++)
   //   {
   //      int byteIndex = random.Next((int)metadataFileSize);
   //      await this.flipBitAndVerifyCorruptionAsync(
   //         random,
   //         byteIndex,
   //         string.Format(
   //            "MetadataManager file should catch data corruption if byte {0} is flipped.",
   //            byteIndex)).ConfigureAwait(false);
   //   }
   //}

   BOOST_AUTO_TEST_SUITE_END()
}
