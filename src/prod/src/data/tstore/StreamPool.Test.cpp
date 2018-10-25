// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ALLOC_TAG 'tpTS'

namespace TStoreTests
{
   using namespace ktl;
   using namespace Data::TStore;
   using namespace Data::TestCommon;

   class StreamPoolTest
   {
   public:
      StreamPoolTest()
      {
         NTSTATUS status;
         status = KtlSystem::Initialize(FALSE, &ktlSystem_);
         CODING_ERROR_ASSERT(NT_SUCCESS(status));
         ktlSystem_->SetStrictAllocationChecks(TRUE);

         fileNames = _new(ALLOC_TAG, GetAllocator()) KSharedArray<KString::SPtr>();
      }

      ~StreamPoolTest()
      {
          for (ULONG32 i = 0; i < fileNames->Count(); i++)
          {
              KString::SPtr fileName = (*fileNames)[i];
              Common::File::Delete(fileName->operator LPCWSTR());
          }

          fileNames = nullptr;

          ktlSystem_->Shutdown();
      }

      KAllocator& GetAllocator()
      {
         return ktlSystem_->NonPagedAllocator();
      }

      KString::SPtr CreateGuidString()
      {
         KGuid guid;
         guid.CreateNew();
         KString::SPtr stringSPtr = nullptr;
         NTSTATUS status = KString::Create(stringSPtr, GetAllocator(), KStringView::MaxGuidString);
         CODING_ERROR_ASSERT(NT_SUCCESS(status));
         BOOLEAN result = stringSPtr->FromGUID(guid);
         CODING_ERROR_ASSERT(result == TRUE);

         return stringSPtr;
      }

      KString::SPtr CreateFileString(__in KStringView & name)
      {
         KString::SPtr fileName;
         KAllocator& allocator = GetAllocator();

         WCHAR currentDirectoryPathCharArray[MAX_PATH];
         GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

#if !defined(PLATFORM_UNIX)
         NTSTATUS status = KString::Create(fileName, allocator, L"\\??\\");
#else
         NTSTATUS status = KString::Create(fileName, allocator, L"");
#endif
         CODING_ERROR_ASSERT(NT_SUCCESS(status));

         BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray);
         CODING_ERROR_ASSERT(concatSuccess == TRUE);

         concatSuccess = fileName->Concat(Common::Path::GetPathSeparatorWstr().c_str());
         CODING_ERROR_ASSERT(concatSuccess == TRUE);

         concatSuccess = fileName->Concat(name);
         CODING_ERROR_ASSERT(concatSuccess == TRUE);

         return fileName.RawPtr();
      }

      ktl::Awaitable<ktl::io::KFileStream::SPtr> CreateFileStream()
      {
         auto fileNameSPtr = CreateGuidString();
         auto fullFileNameSPtr = CreateFileString(*fileNameSPtr);
         KWString fileName(GetAllocator(), *fullFileNameSPtr);

         fileNames->Append(fullFileNameSPtr);

         KBlockFile::SPtr fileSPtr = nullptr;
         ktl::io::KFileStream::SPtr streamSPtr_ = nullptr;

         NTSTATUS status = co_await FileFormatTestHelper::CreateBlockFile(fileName, GetAllocator(), CancellationToken::None, fileSPtr, streamSPtr_);
         VERIFY_IS_TRUE(NT_SUCCESS(status));

         status = co_await(streamSPtr_->OpenAsync(*fileSPtr));
         VERIFY_IS_TRUE(NT_SUCCESS(status));

         streamsCreated_++;

         co_return streamSPtr_;
      }

      ULONG32 streamsCreated_ = 0;

   private:
      KtlSystem* ktlSystem_;
      KSharedArray<KString::SPtr>::SPtr fileNames;

#pragma region test functions
    public:
        ktl::Awaitable<void> NewPool_ShouldBeEmpty_Test()
       {
          StreamPool::SPtr streamPool = nullptr;
          StreamPool::Create(StreamPool::StreamFactoryType(this, &StreamPoolTest::CreateFileStream), GetAllocator(), streamPool, 2);
          ASSERT_IFNOT(streamPool != nullptr, "stream pool cannot be null");

          ASSERT_IFNOT(streamPool->Count() == 0, "stream pool should be empty");

          co_await streamPool->CloseAsync();
           co_return;
       }

        ktl::Awaitable<void> AcquireStream_OneCaller_ShouldCreateOneNewStream_Test()
       {
          StreamPool::SPtr streamPool = nullptr;
          StreamPool::Create(StreamPool::StreamFactoryType(this, &StreamPoolTest::CreateFileStream), GetAllocator(), streamPool, 8);
          ASSERT_IFNOT(streamPool != nullptr, "stream pool cannot be null");
      
          // First AcquireStream() call will create a new stream.
          auto stream1 = co_await streamPool->AcquireStreamAsync();
      
          ASSERT_IFNOT(stream1 != nullptr, "stream1 cannot be null");
          ASSERT_IFNOT(streamPool->Count() == 0, "StreamPool should not still be holding onto the stream it returned.");
          ASSERT_IFNOT(streamsCreated_ == 1, "StreamPool should have created a stream by calling the stream creation function.");

          // ReleaseStream() should put the stream back into the pool.
          co_await streamPool->ReleaseStreamAsync(*stream1);

          ASSERT_IFNOT(streamPool->Count() == 1, "StreamPool.ReleaseStream() should now be holding onto the released stream.");
          ASSERT_IFNOT(streamsCreated_ ==  1, "StreamPool.ReleaseStream() should not have created additional streams.");

          // Second AcquireStream() should re-acquire the same stream withour re-creating one.
          auto stream2 = co_await streamPool->AcquireStreamAsync();

          ASSERT_IFNOT(stream2 != nullptr, "StreamPool.AcquireStream() returned null.");
          ASSERT_IFNOT(stream1 == stream2, "StreamPool.AcquireStream() did not return the same instance.");
          ASSERT_IFNOT(streamPool->Count() == 0, "StreamPool.AcquireStream() should not still be holding onto the streams it returned.");

          // Release the stream again and validate
          co_await streamPool->ReleaseStreamAsync(*stream2);

          ASSERT_IFNOT(streamPool->Count() == 1, "StreamPool.ReleaseStream() should now be holding onto the released stream.");
          ASSERT_IFNOT(streamsCreated_ == 1, "StreamPool.ReleaseStream() should not have created additional streams after second call.");

          co_await streamPool->CloseAsync();
           co_return;
       }

        ktl::Awaitable<void> AcquireStream_TwoCallers_ShouldCreateTwoNewStreams_Test()
       {
          StreamPool::SPtr streamPool = nullptr;
          StreamPool::Create(StreamPool::StreamFactoryType(this, &StreamPoolTest::CreateFileStream), GetAllocator(), streamPool, 8);
          ASSERT_IFNOT(streamPool != nullptr, "stream pool cannot be null");

          // Acquire 2 streams concurrently
          auto stream1 = co_await streamPool->AcquireStreamAsync();
          auto stream2 = co_await streamPool->AcquireStreamAsync();

          ASSERT_IFNOT(stream1 != nullptr, "StreamPool.AcquireStream() returned null for stream1.");
          ASSERT_IFNOT(stream2 != nullptr, "StreamPool.AcquireStream() returned null for stream2.");
          ASSERT_IFNOT(stream1 != stream2, "StreamPool.AcquireStream() should return unique instances when called concurrently.");
          ASSERT_IFNOT(streamPool->Count() == 0, "StreamPool should not still be holding onto the streams it returned.");
          ASSERT_IFNOT(streamsCreated_ == 2, "StreamPool.AcquireStream() should have created unique instances when called concurrently.");

          // Release 2 streams.
          co_await streamPool->ReleaseStreamAsync(*stream1);
          co_await streamPool->ReleaseStreamAsync(*stream2);

          ASSERT_IFNOT(streamPool->Count() == 2, "StreamPool should now be holding onto the streams it returned.");
          ASSERT_IFNOT(streamsCreated_ == 2, "StreamPool.ReleaseStream() should have created unique instances when called concurrently.");

          // Re-acquire the 2 streams.
          auto stream3 = co_await streamPool->AcquireStreamAsync();
          auto stream4 = co_await streamPool->AcquireStreamAsync();

          ASSERT_IFNOT(stream3 != nullptr, "StreamPool.AcquireStream() returned null for stream3.");
          ASSERT_IFNOT(stream4 != nullptr, "StreamPool.AcquireStream() returned null for stream4.");
          ASSERT_IFNOT(stream3 != stream4, "StreamPool.AcquireStream() should return unique instances when called concurrently.");
          ASSERT_IFNOT((stream3 == stream1) || (stream3 == stream2), "StreamPool.AcquireStream() should have returned one of the original streams for stream3.");
          ASSERT_IFNOT((stream4 == stream1) || (stream4 == stream2), "StreamPool.AcquireStream() should have returned one of the original streams for stream4.");
          ASSERT_IFNOT(streamPool->Count() == 0, "StreamPool.AcquireStream() should not still be holding onto the streams it returned.");

          // Release the 2 streams.
          co_await streamPool->ReleaseStreamAsync(*stream3);
          co_await streamPool->ReleaseStreamAsync(*stream4);

          ASSERT_IFNOT(streamPool->Count() == 2, "StreamPool.ReleaseStream() should now be holding onto the released streams.");
          ASSERT_IFNOT(streamsCreated_ == 2, "StreamPool.ReleaseStream() should not have created additional streams after second set of calls.");

          co_await streamPool->CloseAsync();
           co_return;
       }

        ktl::Awaitable<void> ReleaseStream_MoreStreamsThanDesiredMax_ShouldDropStreams_Test()
       {
          StreamPool::SPtr streamPool = nullptr;
          StreamPool::Create(StreamPool::StreamFactoryType(this, &StreamPoolTest::CreateFileStream), GetAllocator(), streamPool, 2);
          ASSERT_IFNOT(streamPool != nullptr, "stream pool cannot be null");

          // Acquire streams.
          auto stream1 = co_await streamPool->AcquireStreamAsync();
          auto stream2 = co_await streamPool->AcquireStreamAsync();
          auto stream3 = co_await streamPool->AcquireStreamAsync();
          auto stream4 = co_await streamPool->AcquireStreamAsync();
          auto stream5 = co_await streamPool->AcquireStreamAsync();
          auto stream6 = co_await streamPool->AcquireStreamAsync();

          // Release 2 streams (max stream count is 2).
          co_await streamPool->ReleaseStreamAsync(*stream1);
          co_await streamPool->ReleaseStreamAsync(*stream2);

          // The desired max stream count is 2, so every other stream after that should get dropped.
          co_await streamPool->ReleaseStreamAsync(*stream3);
          ASSERT_IFNOT(streamPool->Count() == 2, "StreamPool.ReleaseStream() should have dropped the 3rd stream since the pool exceeded the max desired stream count.");
          co_await streamPool->ReleaseStreamAsync(*stream4);
          ASSERT_IFNOT(streamPool->Count() == 3, "StreamPool.ReleaseStream() should have reused the 4th stream.");
          co_await streamPool->ReleaseStreamAsync(*stream5);
          ASSERT_IFNOT(streamPool->Count() == 3, "StreamPool.ReleaseStream() should have dropped the 5th stream.");
          co_await streamPool->ReleaseStreamAsync(*stream6);
          ASSERT_IFNOT(streamPool->Count() == 4, "StreamPool.ReleaseStream() should have reused the 6th stream.");

          co_await streamPool->CloseAsync();
           co_return;
       }
    #pragma endregion
   };

   BOOST_FIXTURE_TEST_SUITE(StreamPoolTestSuite, StreamPoolTest)

      BOOST_AUTO_TEST_CASE(NewPool_ShouldBeEmpty)
   {
       SyncAwait(NewPool_ShouldBeEmpty_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireStream_OneCaller_ShouldCreateOneNewStream)
   {
       SyncAwait(AcquireStream_OneCaller_ShouldCreateOneNewStream_Test());
   }

   BOOST_AUTO_TEST_CASE(AcquireStream_TwoCallers_ShouldCreateTwoNewStreams)
   {
       SyncAwait(AcquireStream_TwoCallers_ShouldCreateTwoNewStreams_Test());
   }

   BOOST_AUTO_TEST_CASE(ReleaseStream_MoreStreamsThanDesiredMax_ShouldDropStreams)
   {
       SyncAwait(ReleaseStream_MoreStreamsThanDesiredMax_ShouldDropStreams_Test());
   }

   BOOST_AUTO_TEST_SUITE_END()
}
