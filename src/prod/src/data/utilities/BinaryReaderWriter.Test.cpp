// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace UtilitiesTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class BinaryReaderWriterTest
    {
    public:
        Common::CommonConfig config; // load the config object as its needed for the tracing to work
        Common::StringLiteral const BinaryReaderWriterTestTrace = "BinaryReaderWriterTest";

        BinaryReaderWriterTest()
        {
            NTSTATUS status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        void Test_ReadWrite(
            __in KString & writeString,
            __in Encoding encoding);

        void Test_ReadWrite(
            __in KStringA & writeString);

        ~BinaryReaderWriterTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->PagedAllocator();
        }

    public:
        KtlSystem* ktlSystem_;
    };

    void BinaryReaderWriterTest::Test_ReadWrite(
        __in KString & writeString, 
        __in Encoding encoding)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        BinaryWriter writer(allocator);
        writer.Write(writeString, encoding);
        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        KString::SPtr readString = KString::Create(allocator);
        reader.Read(readString, encoding);

        CODING_ERROR_ASSERT(writeString.Compare(*readString) == 0);
    }

    void BinaryReaderWriterTest::Test_ReadWrite(
        __in KStringA & writeString)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        BinaryWriter writer(allocator);
        writer.Write(writeString);
        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        KStringA::SPtr readString = KStringA::Create(allocator);
        reader.Read(readString);

        CODING_ERROR_ASSERT(writeString.Compare(*readString) == 0);
    }

    BOOST_FIXTURE_TEST_SUITE(BinaryReaderWriterTestSuite, BinaryReaderWriterTest)

    BOOST_AUTO_TEST_CASE(BasicInt)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();
        int writeValue = MININT;
        int readValue = 0;

        BinaryWriter writer(allocator);

        writer.Write(writeValue);

        auto buffer = writer.GetBuffer(0, sizeof(writeValue));

        BinaryReader reader(*buffer, allocator);
        reader.Read(readValue);

        CODING_ERROR_ASSERT(readValue == writeValue);

        writeValue = MAXINT;
        writer.Position = 0;

        writer.Write(writeValue);

        buffer = writer.GetBuffer(0, sizeof(writeValue));
        BinaryReader reader2(*buffer, allocator);

        reader2.Read(readValue);

        CODING_ERROR_ASSERT(readValue == writeValue);
    }

    BOOST_AUTO_TEST_CASE(BasicUInt)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();
        int writeValue = MAXUINT;
        int readValue = 0;

        BinaryWriter writer(allocator);

        writer.Write(writeValue);

        auto buffer = writer.GetBuffer(0, sizeof(writeValue));

        BinaryReader reader(*buffer, allocator);
        reader.Read(readValue);

        CODING_ERROR_ASSERT(readValue == writeValue);
    }

    BOOST_AUTO_TEST_CASE(BasicLONG64)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();
        LONG64 writeValue = MAXLONG64;
        LONG64 readValue = 0;

        BinaryWriter writer(allocator);

        writer.Write(writeValue);

        auto buffer = writer.GetBuffer(0, sizeof(writeValue));

        BinaryReader reader(*buffer, allocator);
        reader.Read(readValue);

        CODING_ERROR_ASSERT(readValue == writeValue);

        writeValue = MINLONG64;
        writer.Position = 0;

        writer.Write(writeValue);

        buffer = writer.GetBuffer(0, sizeof(writeValue));
        BinaryReader reader2(*buffer, allocator);

        reader2.Read(readValue);

        CODING_ERROR_ASSERT(readValue == writeValue);
    }

    BOOST_AUTO_TEST_CASE(BasicULONG64)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();
        ULONG64 writeValue = MAXULONG64;
        ULONG64 readValue = 0;

        BinaryWriter writer(allocator);

        writer.Write(writeValue);

        auto buffer = writer.GetBuffer(0, sizeof(writeValue));

        BinaryReader reader(*buffer, allocator);
        reader.Read(readValue);

        CODING_ERROR_ASSERT(readValue == writeValue);
    }

    BOOST_AUTO_TEST_CASE(AllIntegerTypes)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();
        INT intWriteValue = MAXINT;
        UINT uintWriteValue = MAXUINT;
        LONG64 longWriteValue = MAXLONG;
        ULONG64 ulongWriteValue = MAXULONG64;

        INT intReadValue = 0;
        UINT uintReadValue = 0;
        LONG64 longReadValue = 0;
        ULONG64 ulongReadValue = 0;

        BinaryWriter writer(allocator);
        
        writer.Write(ulongWriteValue);
        writer.Write(longWriteValue);
        writer.Write(uintWriteValue);
        writer.Write(intWriteValue);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);

        reader.Read(ulongReadValue);
        reader.Read(longReadValue);
        reader.Read(uintReadValue);
        reader.Read(intReadValue);

        CODING_ERROR_ASSERT(ulongWriteValue == ulongReadValue);
        CODING_ERROR_ASSERT(longWriteValue == longReadValue);
        CODING_ERROR_ASSERT(uintWriteValue == uintReadValue);
        CODING_ERROR_ASSERT(intWriteValue == intReadValue);
    }
 
    BOOST_AUTO_TEST_CASE(KString_Basic_UTF8_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KString::SPtr basicString;
        NTSTATUS status = KString::Create(basicString, allocator, L"fabric:/sampleapp/app");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString, UTF8);
    }

    BOOST_AUTO_TEST_CASE(KString_Basic_UTF16_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KString::SPtr basicString;
        NTSTATUS status = KString::Create(basicString, allocator, L"fabric:/sampleapp/app");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString, UTF16);
    }

    BOOST_AUTO_TEST_CASE(KStringA_Basic_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KStringA::SPtr basicString;
        NTSTATUS status = KStringA::Create(basicString, allocator, "fabric:/sampleapp/app");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString);
    }

    BOOST_AUTO_TEST_CASE(KString_Empty_UTF8_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KString::SPtr basicString;
        NTSTATUS status = KString::Create(basicString, allocator, L"");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString, UTF8);
    }

    BOOST_AUTO_TEST_CASE(KString_Empty_UTF16_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KString::SPtr basicString;
        NTSTATUS status = KString::Create(basicString, allocator, L"");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString, UTF16);
    }

    BOOST_AUTO_TEST_CASE(KStringA_Empty_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KStringA::SPtr basicString;
        NTSTATUS status = KStringA::Create(basicString, allocator, "");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString);
    }

    BOOST_AUTO_TEST_CASE(KString_1Char_UTF8_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KString::SPtr basicString;
        NTSTATUS status = KString::Create(basicString, allocator, L"z");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString, UTF8);
    }

    BOOST_AUTO_TEST_CASE(KString_1Char_UTF16_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KString::SPtr basicString;
        NTSTATUS status = KString::Create(basicString, allocator, L"z");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString, UTF16);
    }

    BOOST_AUTO_TEST_CASE(KStringA_1Char_Success)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KStringA::SPtr basicString;
        NTSTATUS status = KStringA::Create(basicString, allocator, "z");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        Test_ReadWrite(*basicString);
    }

    BOOST_AUTO_TEST_CASE(KUri_SUCCESS)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KUri::SPtr writeUri;
        NTSTATUS status = KUri::Create(L"fabric:/sampleapp/app", allocator, writeUri);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BinaryWriter writer(allocator);
        writer.Write(*writeUri);
        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        KUri::SPtr readUri;
        reader.Read(readUri);

        CODING_ERROR_ASSERT(writeUri->Compare(*readUri) == TRUE);
    }

    BOOST_AUTO_TEST_CASE(LargeString)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();
        int stringSize = (128 * 1024) - 3;

        KString::SPtr writeString;
        NTSTATUS status = KString::Create(writeString, allocator, L""); 
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (int i = 0; i < stringSize; i++)
        {
            switch (i % 3)
            {
            case 0:
                writeString->AppendChar(L'x');
                break;
            case 1:
                writeString->AppendChar(L'o');
                break;
            case 2:
                writeString->AppendChar(L'r');
                break;
            default:
                CODING_ERROR_ASSERT(false);
                break;
            }
        }

        writeString->SetNullTerminator();
        BinaryWriter writer(allocator);
        KString const & writtenString = *writeString;
        writer.Write(writtenString);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        KString::SPtr readString = KString::Create(allocator);
        reader.Read(readString);
        
        CODING_ERROR_ASSERT(writeString->Compare(*readString) == 0);
    }

    BOOST_AUTO_TEST_CASE(UNICODE_MultipleCharacters_SUCCESS)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        KString::SPtr writeString;
        NTSTATUS status = KString::Create(writeString, allocator, L"urn:/᚛᚛ᚉᚑᚅᚔᚉᚉᚔᚋ ᚔᚈᚔ ᚍᚂᚐᚅᚑ ᚅᚔᚋᚌᚓᚅᚐ᚜ ᛖᚴ ᚷᛖᛏ ᛖᛏᛁ ᚧ ᚷᛚᛖᚱ ᛘᚾ ᚦᛖᛋᛋ ᚨᚧ ᚡᛖ ᚱᚧᚨ ᛋᚨᚱ Cam yiyebilirim, bana zararı dokunmaz. جام ييه بلورم بڭا ضررى طوقونمز");
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        writeString->SetNullTerminator();
        BinaryWriter writer(allocator);
        KString const & writtenString = *writeString;
        writer.Write(writtenString);

        auto buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        KString::SPtr readString = KString::Create(allocator);
        reader.Read(readString);

        CODING_ERROR_ASSERT(writeString->Compare(*readString) == 0);
    }

    BOOST_AUTO_TEST_CASE(WriteInBytes_ReadByBytes_LONG32)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();
        LONG32 writeValue1 = MINLONG32;
        LONG32 writeValue2 = MINLONG32 / 2;
        LONG32 writeValue3 = 0;
        LONG32 writeValue4 = MAXLONG32 / 2;
        LONG32 writeValue5 = MAXLONG32;

        BinaryWriter writer(allocator);
        CODING_ERROR_ASSERT(NT_SUCCESS(writer.Status()));

        NTSTATUS status = STATUS_UNSUCCESSFUL;
        status = writer.WriteInBytes(writeValue1);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = writer.WriteInBytes(writeValue2);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = writer.WriteInBytes(writeValue3);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = writer.WriteInBytes(writeValue4);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = writer.WriteInBytes(writeValue5);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(writer.Position == sizeof(LONG32) * 5);

        KBuffer::SPtr buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        CODING_ERROR_ASSERT(NT_SUCCESS(reader.Status()));

        LONG32 readValue1 = 0;
        LONG32 readValue2 = 0;
        LONG32 readValue3 = 0;
        LONG32 readValue4 = 0;
        LONG32 readValue5 = 0;

        status = reader.ReadByBytes(readValue1);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = reader.ReadByBytes(readValue2);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = reader.ReadByBytes(readValue3);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = reader.ReadByBytes(readValue4);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = reader.ReadByBytes(readValue5);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(writeValue1 == readValue1);
        CODING_ERROR_ASSERT(writeValue2 == readValue2);
        CODING_ERROR_ASSERT(writeValue3 == readValue3);
        CODING_ERROR_ASSERT(writeValue4 == readValue4);
        CODING_ERROR_ASSERT(writeValue5 == readValue5);
    }

    BOOST_AUTO_TEST_CASE(WriteInBytes_ReadByBytes_LONG64)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();
        LONG64 writeValue1 = MINLONG64;
        LONG64 writeValue2 = MINLONG64 / 2;
        LONG64 writeValue3 = 0;
        LONG64 writeValue4 = MAXLONG64 / 2;
        LONG64 writeValue5 = MAXLONG64;

        BinaryWriter writer(allocator);
        CODING_ERROR_ASSERT(NT_SUCCESS(writer.Status()));

        NTSTATUS status = STATUS_UNSUCCESSFUL;
        status = writer.WriteInBytes(writeValue1);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = writer.WriteInBytes(writeValue2);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = writer.WriteInBytes(writeValue3);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = writer.WriteInBytes(writeValue4);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = writer.WriteInBytes(writeValue5);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(writer.Position == sizeof(LONG64) * 5);

        KBuffer::SPtr buffer = writer.GetBuffer(0, writer.Position);

        BinaryReader reader(*buffer, allocator);
        CODING_ERROR_ASSERT(NT_SUCCESS(reader.Status()));

        LONG64 readValue1 = 0;
        LONG64 readValue2 = 0;
        LONG64 readValue3 = 0;
        LONG64 readValue4 = 0;
        LONG64 readValue5 = 0;

        status = reader.ReadByBytes(readValue1);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = reader.ReadByBytes(readValue2);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = reader.ReadByBytes(readValue3);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = reader.ReadByBytes(readValue4);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = reader.ReadByBytes(readValue5);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        CODING_ERROR_ASSERT(writeValue1 == readValue1);
        CODING_ERROR_ASSERT(writeValue2 == readValue2);
        CODING_ERROR_ASSERT(writeValue3 == readValue3);
        CODING_ERROR_ASSERT(writeValue4 == readValue4);
        CODING_ERROR_ASSERT(writeValue5 == readValue5);
    }

    BOOST_AUTO_TEST_CASE(Verify_Padding_Cursor_Behavior)
    {
        KAllocator& allocator = BinaryReaderWriterTest::GetAllocator();

        BinaryWriter writer(allocator);
        CODING_ERROR_ASSERT(NT_SUCCESS(writer.Status()));

        Trace.WriteInfo(
            BinaryReaderWriterTestTrace,
            "Initial writer.Position = {0}, writer.Size = {1}",
            writer.Position,
            writer.Test_Size);

        unsigned char dummy = '\0';
        ULONG targetWriterSize = 93;
        ULONG finalCursorPosition = targetWriterSize + 3;

        while (writer.Test_Size != targetWriterSize)
        {
            writer.Write(&dummy);

            Trace.WriteInfo(
                BinaryReaderWriterTestTrace,
                "writer.Position = {0}, writer.Size = {1}",
                writer.Position,
                writer.Test_Size);
        }

        Trace.WriteInfo(
            BinaryReaderWriterTestTrace,
            "Final writer.Position = {0}, writer.Size = {1}",
            writer.Position,
            writer.Test_Size);

        CODING_ERROR_ASSERT(writer.Test_Size == targetWriterSize);
        CODING_ERROR_ASSERT(writer.Test_Size == writer.Position);

        
        // Set the cursor to 1 below the current total size
        // Current size = 93, Cursor position = 92
        writer.Position = targetWriterSize - 1;

        /*
         #11444928:Throwing coding error - BinaryWriter:set_Position memchannel cursor 95 is not equal to value 96 

         Repro:
         1. Set the position to target size + 3 (96), ensuring that padding is necessary
         2. Difference will be calculated as 3, 3 KMemChannel::Write operations performed
         3. Cursor position would be incremented by 3. 1 for every KMemChannel::Write.
         4. Final cursor position = 95, not 96. and BinaryWriter asserts that the target position != cursor

         Fix:
          Set cursor position == total size if padding is necessary, ensuring cursor position aligns with total size
        */

        writer.Position = finalCursorPosition;
        CODING_ERROR_ASSERT(writer.Position == finalCursorPosition);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
