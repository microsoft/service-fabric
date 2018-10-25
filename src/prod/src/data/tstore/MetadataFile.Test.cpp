// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace ktl;

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;
    using namespace Data::TestCommon;

    struct MetadataFileTest
    {
        MetadataFileTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~MetadataFileTest()
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

    private:
        KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> Properties_WriteRead_ShouldSucceed_Test()
        {
           KAllocator& allocator = MetadataFileTest::GetAllocator();

           BlockHandle::SPtr blockHandle = nullptr;
           NTSTATUS status = BlockHandle::Create(0, 0, allocator, blockHandle);
           CODING_ERROR_ASSERT(NT_SUCCESS(status));

           MetadataManagerFileProperties::SPtr metadataManagerFilePropertiesSPtr = nullptr;
           status = MetadataManagerFileProperties::Create(allocator, metadataManagerFilePropertiesSPtr);
           CODING_ERROR_ASSERT(NT_SUCCESS(status));

           metadataManagerFilePropertiesSPtr->MetadataHandleSPtr = *blockHandle;
       
           KBlockFile::SPtr fakeFileSPtr = nullptr;
           ktl::io::KFileStream::SPtr fakeStream = nullptr;

           WCHAR currentDirectoryPathCharArray[MAX_PATH];
           GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);
           PathAppendW(currentDirectoryPathCharArray, L"WriteBlockAsyncTest.txt");

    #if !defined(PLATFORM_UNIX)
           KWString fileName(allocator, L"\\??\\");
    #else
           KWString fileName(allocator, L"");
    #endif
           VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));

           KWString currentFilePath(allocator, currentDirectoryPathCharArray);
           VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));

           fileName += currentFilePath;

           status = co_await FileFormatTestHelper::CreateBlockFile(fileName, allocator, CancellationToken::None, fakeFileSPtr, fakeStream);
           VERIFY_IS_TRUE(NT_SUCCESS(status));

           status = co_await fakeStream->OpenAsync(*fakeFileSPtr);
           VERIFY_IS_TRUE(NT_SUCCESS(status));

           BlockHandle::SPtr writeHandle = nullptr;

           FileBlock<MetadataManagerFileProperties::SPtr>::SerializerFunc serializerFunction(metadataManagerFilePropertiesSPtr.RawPtr(), &MetadataManagerFileProperties::Write);
           status = co_await FileBlock<MetadataManagerFileProperties::SPtr>::WriteBlockAsync(*fakeStream, serializerFunction, allocator, ktl::CancellationToken::None, writeHandle);

           CODING_ERROR_ASSERT(NT_SUCCESS(status));
           CODING_ERROR_ASSERT(writeHandle->Offset == 0);
           CODING_ERROR_ASSERT(writeHandle->Size == 43);

           CODING_ERROR_ASSERT(metadataManagerFilePropertiesSPtr->MetadataHandleSPtr->Offset == 0);
           CODING_ERROR_ASSERT(metadataManagerFilePropertiesSPtr->MetadataHandleSPtr->Size == 0);

           MetadataManagerFileProperties::SPtr readHandle = nullptr;

           FileBlock<MetadataManagerFileProperties::SPtr>::DeserializerFunc deserializerFuncFunction(&MetadataManagerFileProperties::Read);
           readHandle = co_await FileBlock<MetadataManagerFileProperties::SPtr>::ReadBlockAsync(*fakeStream, *writeHandle, deserializerFuncFunction, allocator, ktl::CancellationToken::None);

           CODING_ERROR_ASSERT(readHandle->MetadataHandleSPtr->Offset == 0);
           CODING_ERROR_ASSERT(readHandle->MetadataHandleSPtr->Size == 0); 

           status = co_await fakeStream->CloseAsync();
           CODING_ERROR_ASSERT(NT_SUCCESS(status));

           fakeFileSPtr->Close();

           status = KVolumeNamespace::DeleteFileOrDirectory(fileName, GetAllocator(), nullptr);
           CODING_ERROR_ASSERT(NT_SUCCESS(status));
            co_return;
        }

        ktl::Awaitable<void> FileMetdata_WriteRead_ShouldSucceed_Test()
        {
            KAllocator& allocator = MetadataFileTest::GetAllocator();

            FileMetadata::SPtr fileMeta = nullptr;
            KString::SPtr fileNameSPtr = nullptr;
            NTSTATUS status = KString::Create(fileNameSPtr, allocator, L"test1");
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            status = FileMetadata::Create(1, *fileNameSPtr, 2, 2, 2, 2, false, allocator, *CreateTraceComponent(), fileMeta);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BinaryWriter writer(allocator);

            fileMeta->Write(writer);

            auto buffer = writer.GetBuffer(0, writer.Position);
            BinaryReader reader(*buffer, allocator);
            FileMetadata::SPtr readFileMetdataSPtr = FileMetadata::Read(reader, allocator, *CreateTraceComponent());

            CODING_ERROR_ASSERT(readFileMetdataSPtr->NumberOfDeletedEntries == 2);
            CODING_ERROR_ASSERT(readFileMetdataSPtr->FileName->Compare(*fileNameSPtr) == 0);
            CODING_ERROR_ASSERT(readFileMetdataSPtr->FileId == 1);
            CODING_ERROR_ASSERT(readFileMetdataSPtr->NumberOfValidEntries == 2);

            co_await fileMeta->CloseAsync();
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(MetadataFileTestSuite, MetadataFileTest);
    
    BOOST_AUTO_TEST_CASE(Properties_WriteRead_ShouldSucceed)
    {
        SyncAwait(Properties_WriteRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(FileMetdata_WriteRead_ShouldSucceed)
    {
        SyncAwait(FileMetdata_WriteRead_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
