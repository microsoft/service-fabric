// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "../testcommon/TestCommon.Public.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;
    using namespace Data::TestCommon;

    class FileFormatTest
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

        FileFormatTest()
        {
            NTSTATUS status;
            status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~FileFormatTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

    private:
        KtlSystem* ktlSystem_;
    };

    BOOST_FIXTURE_TEST_SUITE(FileFormatTestSuite, FileFormatTest)

    BOOST_AUTO_TEST_CASE(BlockHandle_CreateAndGetter)
    {
        NTSTATUS status;
        ULONG64 offset = 10;
        ULONG64 size = 20;

        KAllocator& allocator = FileFormatTest::GetAllocator();

        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(blockHandle->Offset == 10);
        CODING_ERROR_ASSERT(blockHandle->Size == 20);
    }
    
    BOOST_AUTO_TEST_CASE(BlockHandle_CreateAndGetter_MaxValue)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = MAXLONG64;
        ULONG64 size = MAXLONG64;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(blockHandle->Offset == MAXLONG64);
        CODING_ERROR_ASSERT(blockHandle->Size == MAXLONG64);
    }

    BOOST_AUTO_TEST_CASE(BlockHandle_SerializedSize)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;

        ULONG32 expectedSize = sizeof(ULONG64) + sizeof(ULONG64);

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(blockHandle->SerializedSize() == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(BlockHandle_EndOffset)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG64 expectedSize = 30;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CODING_ERROR_ASSERT(blockHandle->EndOffset() == expectedSize);
    }

    BOOST_AUTO_TEST_CASE(BlockHandle_Write)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;

        NTSTATUS status;

        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ULONG64 readOffset = 0;
        ULONG64 readSize = 0;

        BinaryWriter writer(allocator);

        blockHandle->Write(writer);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        reader.Read(readOffset);
        reader.Read(readSize);

        CODING_ERROR_ASSERT(readOffset == 10);
        CODING_ERROR_ASSERT(readSize == 20);
    }

    BOOST_AUTO_TEST_CASE(BlockHandle_Read)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;

        NTSTATUS status; 
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        BinaryWriter writer(allocator);

        blockHandle->Write(writer);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);

        BlockHandle::SPtr result = blockHandle->Read(reader, allocator);

        CODING_ERROR_ASSERT(result->Offset == 10);
        CODING_ERROR_ASSERT(result->Size == 20);
    }

    BOOST_AUTO_TEST_CASE(FileFooter_CreateAndGetter)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 1;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(blockHandle->Offset == 10);
        CODING_ERROR_ASSERT(blockHandle->Size == 20);

        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(fileFooter->Version == 1);
        CODING_ERROR_ASSERT(fileFooter->PropertiesHandle->Offset == 10);
        CODING_ERROR_ASSERT(fileFooter->PropertiesHandle->Size == 20);
    }

    BOOST_AUTO_TEST_CASE(FileFooter_CreateAndGetter_MaxValue)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = MAXLONG64;
        ULONG64 size = MAXLONG64;
        ULONG32 version = MAXINT;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(blockHandle->Offset == MAXLONG64);
        CODING_ERROR_ASSERT(blockHandle->Size == MAXLONG64);

        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(fileFooter->Version == MAXINT);
        CODING_ERROR_ASSERT(fileFooter->PropertiesHandle->Offset == MAXLONG64);
        CODING_ERROR_ASSERT(fileFooter->PropertiesHandle->Size == MAXLONG64);
    }

    BOOST_AUTO_TEST_CASE(FileFooter_Get_Version)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 5;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(blockHandle->Offset == 10);
        CODING_ERROR_ASSERT(blockHandle->Size == 20);

        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(fileFooter->Version == 5);
    }

    BOOST_AUTO_TEST_CASE(FileFooter_SerializedSize)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 5;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(blockHandle->Offset == 10);
        CODING_ERROR_ASSERT(blockHandle->Size == 20);

        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ULONG32 expectResult = sizeof(ULONG64) + sizeof(ULONG64) + sizeof(ULONG32);
        CODING_ERROR_ASSERT(fileFooter->SerializedSize() == expectResult);
    }

    BOOST_AUTO_TEST_CASE(FileFooter_Get_PropertiesHandle)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 5;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(blockHandle->Offset == 10);
        CODING_ERROR_ASSERT(blockHandle->Size == 20);

        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(fileFooter->get_PropertiesHandle()->Offset == 10);
        CODING_ERROR_ASSERT(fileFooter->get_PropertiesHandle()->Size == 20);
    }

    BOOST_AUTO_TEST_CASE(FileFooter_Write)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 5;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ULONG64 readOffset = 0;
        ULONG64 readSize = 0;
        ULONG32 readVersion = 0;

        BinaryWriter writer(allocator);
        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        fileFooter->Write(writer);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        reader.Read(readOffset);
        reader.Read(readSize);
        reader.Read(readVersion);

        CODING_ERROR_ASSERT(readOffset == 10);
        CODING_ERROR_ASSERT(readSize == 20);
        CODING_ERROR_ASSERT(readVersion == 5);
    }

    BOOST_AUTO_TEST_CASE(FileFooter_Read)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 5;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BinaryWriter writer(allocator);
        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        fileFooter->Write(writer);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);

        auto result = fileFooter->Read(reader, *blockHandle, allocator);

        CODING_ERROR_ASSERT(result->PropertiesHandle->Offset == 10);
        CODING_ERROR_ASSERT(result->PropertiesHandle->Size == 20);
        CODING_ERROR_ASSERT(result->Version == 5);
    }

    BOOST_AUTO_TEST_CASE(VarInt_GetSerializedSize)
    {    
        //MinFiveByteValue_268435456
        //MinFourByteValue_2097152
        //MinThreeByteValue_16384
        //MinTwoByteValue_128
        //MoreBytesMask_128
        
        ULONG32 result = VarInt::GetSerializedSize(1);
        CODING_ERROR_ASSERT(result == 1);
        result = VarInt::GetSerializedSize(127);
        CODING_ERROR_ASSERT(result == 1);
        result = VarInt::GetSerializedSize(16383);
        CODING_ERROR_ASSERT(result == 2);
        result = VarInt::GetSerializedSize(2097151);
        CODING_ERROR_ASSERT(result == 3);
        result = VarInt::GetSerializedSize(268435455);
        CODING_ERROR_ASSERT(result == 4);
        result = VarInt::GetSerializedSize(268435457);
        CODING_ERROR_ASSERT(result == 5);
    }

    BOOST_AUTO_TEST_CASE(VarInt_ReadUInt32)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        byte value = 127;

        BinaryWriter writer(allocator);
        writer.Write(value);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);

        CODING_ERROR_ASSERT(reader.Position == 0);

        uint result = VarInt::ReadUInt32(reader);

        CODING_ERROR_ASSERT(result == 127);
    }

    BOOST_AUTO_TEST_CASE(VarInt_WriteRead_UInt)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG32 writeValue = 127;

        BinaryWriter writer(allocator);

        VarInt::Write(writer, writeValue);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);

        ULONG32 readValue = VarInt::ReadUInt32( reader);

        CODING_ERROR_ASSERT(readValue == 127);
    }

    BOOST_AUTO_TEST_CASE(VarInt_WriteRead_UInt_Max)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG32 writeValue = UINT_MAX;

        BinaryWriter writer(allocator);

        VarInt::Write(writer, writeValue);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);

        ULONG32 readValue = VarInt::ReadUInt32(reader);

        CODING_ERROR_ASSERT(readValue == UINT_MAX);
    }
    
    BOOST_AUTO_TEST_CASE(FileBlock_ToUInt64)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 5;

        NTSTATUS status;

        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BinaryWriter writer(allocator);

        writer.Write(offset);
        writer.Write(size);

        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        byte buffer[] = { 0, 12, 1, 0, 0, 0, 0, 2, 0 };

        ULONG64 result = FileBlock<FileFooter>::ToUInt64(buffer, 0);
        CODING_ERROR_ASSERT(result == 144115188075924480);

        result = FileBlock<FileFooter>::ToUInt64(buffer, 1);
        CODING_ERROR_ASSERT(result == 562949953421580);
    }

    BOOST_AUTO_TEST_CASE(FileBlock_FileStream)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();

        KBlockFile::SPtr fakeFileSPtr = nullptr;
        ktl::io::KFileStream::SPtr fakeStream = nullptr;

        WCHAR currentDirectoryPathCharArray[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);
        PathAppendW(currentDirectoryPathCharArray, L"FileStreamTest.txt");

#if !defined(PLATFORM_UNIX)
        KWString fileName(allocator, L"\\??\\");
#else
        KWString fileName(allocator, L"");
#endif
        CODING_ERROR_ASSERT(NT_SUCCESS(fileName.Status()));

        KWString currentFilePath(allocator, currentDirectoryPathCharArray);
        CODING_ERROR_ASSERT(NT_SUCCESS(fileName.Status()));

        fileName += currentFilePath;

        NTSTATUS status = SyncAwait(FileFormatTestHelper::CreateBlockFile(fileName, allocator, CancellationToken::None, fakeFileSPtr, fakeStream));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(fakeStream->OpenAsync(*fakeFileSPtr));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BinaryWriter writer(allocator);
        writer.Write(32);
        writer.Write(16);

        auto buffer = writer.GetBuffer(0);
        ULONG length = writer.get_Position();

        status = SyncAwait(fakeStream->WriteAsync(*buffer, 0, length));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        ULONG BytesRead;
        status = KBuffer::Create(55, buffer, allocator);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        fakeStream->SetPosition(0);

        status = SyncAwait(fakeStream->ReadAsync(*buffer, BytesRead, 0 , length));
        CODING_ERROR_ASSERT(BytesRead == length);

        ULONG32 readValue;
        BinaryReader reader(*buffer, allocator);
        reader.Read(readValue);
        CODING_ERROR_ASSERT(readValue == 32);
        reader.Read(readValue);
        CODING_ERROR_ASSERT(readValue == 16);

        status = SyncAwait(fakeStream->CloseAsync());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = KVolumeNamespace::DeleteFileOrDirectory(fileName, allocator, nullptr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }
    
    BOOST_AUTO_TEST_CASE(FileBlock_WriteBlockAsync)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 1;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        CODING_ERROR_ASSERT(blockHandle->Offset == 10);
        CODING_ERROR_ASSERT(blockHandle->Size == 20);

        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

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
        CODING_ERROR_ASSERT(NT_SUCCESS(fileName.Status()));

        KWString currentFilePath(allocator, currentDirectoryPathCharArray);
        CODING_ERROR_ASSERT(NT_SUCCESS(fileName.Status()));

        fileName += currentFilePath;

        status = SyncAwait(FileFormatTestHelper::CreateBlockFile(fileName, allocator, CancellationToken::None, fakeFileSPtr, fakeStream));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(fakeStream->OpenAsync(*fakeFileSPtr));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BlockHandle::SPtr result = nullptr;

        FileBlock<FileFooter::SPtr>::SerializerFunc serializerFunction(fileFooter.RawPtr(), &FileFooter::Write);
        status = SyncAwait(FileBlock<FileFooter::SPtr>::WriteBlockAsync(*fakeStream, serializerFunction, allocator, CancellationToken::None, result));

        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CODING_ERROR_ASSERT(result->Offset == 0);
        CODING_ERROR_ASSERT(result->Size == 20);

        status = SyncAwait(fakeStream->CloseAsync());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = KVolumeNamespace::DeleteFileOrDirectory(fileName, allocator, nullptr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }
    
    BOOST_AUTO_TEST_CASE(FileBlock_ReadBlockAsync)
    {
        KAllocator& allocator = FileFormatTest::GetAllocator();
        ULONG64 offset = 10;
        ULONG64 size = 20;
        ULONG32 version = 1;

        NTSTATUS status;
        BlockHandle::SPtr blockHandle = nullptr;
        status = BlockHandle::Create(offset, size, allocator, blockHandle);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        CODING_ERROR_ASSERT(blockHandle->Offset == 10);
        CODING_ERROR_ASSERT(blockHandle->Size == 20);

        FileFooter::SPtr fileFooter = nullptr;
        status = FileFooter::Create(*blockHandle, version, allocator, fileFooter);
        CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

        KBlockFile::SPtr fakeFileSPtr = nullptr;
        ktl::io::KFileStream::SPtr fakeStream = nullptr;

        WCHAR currentDirectoryPathCharArray[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);
        PathAppendW(currentDirectoryPathCharArray, L"ReadBlockAsyncTest.txt");

#if !defined(PLATFORM_UNIX)
        KWString fileName(allocator, L"\\??\\");
#else
        KWString fileName(allocator, L"");
#endif
        CODING_ERROR_ASSERT(NT_SUCCESS(fileName.Status()));

        KWString currentFilePath(allocator, currentDirectoryPathCharArray);
        CODING_ERROR_ASSERT(NT_SUCCESS(fileName.Status()));

        fileName += currentFilePath;

        status = SyncAwait(FileFormatTestHelper::CreateBlockFile(fileName, allocator, CancellationToken::None, fakeFileSPtr, fakeStream));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(fakeStream->OpenAsync(*fakeFileSPtr));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BlockHandle::SPtr resultBlockHandle = nullptr;

        FileBlock<FileFooter::SPtr>::SerializerFunc serializerFunction(fileFooter.RawPtr(), &FileFooter::Write);
        status = SyncAwait(FileBlock<FileFooter::SPtr>::WriteBlockAsync(*fakeStream, serializerFunction, allocator, CancellationToken::None, resultBlockHandle));

        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        CODING_ERROR_ASSERT(resultBlockHandle->Offset == 0);
        CODING_ERROR_ASSERT(resultBlockHandle->Size == 20);

        FileFooter::SPtr result = nullptr;

        FileBlock<FileFooter::SPtr>::DeserializerFunc deserializerFuncFunction(&FileFooter::Read);
        result = SyncAwait(FileBlock<FileFooter::SPtr>::ReadBlockAsync(*fakeStream, *resultBlockHandle, deserializerFuncFunction, allocator, CancellationToken::None));

        CODING_ERROR_ASSERT(result->PropertiesHandle->Offset == 10);
        CODING_ERROR_ASSERT(result->PropertiesHandle->Size == 20);
        CODING_ERROR_ASSERT(result->Version == 1);

        status = SyncAwait(fakeStream->CloseAsync());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = KVolumeNamespace::DeleteFileOrDirectory(fileName, allocator, nullptr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
