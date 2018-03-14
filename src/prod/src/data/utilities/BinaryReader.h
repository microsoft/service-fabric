// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <codecvt>

namespace Data
{
    namespace Utilities
    {

#define NULL_KBUFFER_SERIALIZATION_CODE -1
#define BINARYREADER_TAG 'RniB'

        //
        // A Binary Reader abstraction for reading objects from a memory stream
        // Current implementation uses a KInChannel to read the data from a memory channel
        //
        class BinaryReader : public KObject<BinaryReader>
        {
        public:

            //
            // TODO: Remove Copy
            // The readbuffer is copied into the binary reader's underlying memory buffer
            //
            BinaryReader(
                __in KBuffer const & readBuffer,
                __in KAllocator & allocator);

            //
            // Gets/Sets the underlying cursor
            //
            __declspec(property(get = get_Position, put = set_Position)) ULONG Position;
            ULONG get_Position() const
            {
                return memChannel_.GetCursor();
            }

            void set_Position(ULONG value)
            {
                NTSTATUS status = memChannel_.SetCursor(value, KMemChannel::eFromBegin);
                THROW_ON_FAILURE(status);
            }

            __declspec(property(get = get_Length)) ULONG Length;
            ULONG get_Length() const
            {
                return memChannel_.Size();
            }

#pragma region POD BASED TYPES
            void Read(__out bool & value)
            {
                NTSTATUS status = memChannel_.Read(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Read(__out byte & value)
            {
                NTSTATUS status = memChannel_.Read(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Read(__out ULONG32 & value)
            {
                NTSTATUS status = memChannel_.Read(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Read(__out LONG32 & value)
            {
                NTSTATUS status = memChannel_.Read(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Read(__out ULONG64 & value)
            {
                NTSTATUS status = memChannel_.Read(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Read(__out LONG64 & value)
            {
                NTSTATUS status = memChannel_.Read(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Read(__out KGuid & value)
            {
                NTSTATUS status = memChannel_.Read(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }
#pragma endregion

#pragma BitConverter
            // Note: The ReadByBytes method converts the sizeof(LONG32) bytes to an LONG32 value
            // Only consider the little Endian now.
            NTSTATUS ReadByBytes(__out LONG32 & value) noexcept
            {
                byte numArray[sizeof(LONG32)];
                NTSTATUS status = memChannel_.Read(sizeof(LONG32), numArray);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                value = static_cast<LONG32>(*numArray) |
                    static_cast<LONG32>(numArray[1]) << 8 |
                    static_cast<LONG32>(numArray[2]) << 16 |
                    static_cast<LONG32>(numArray[3]) << 24;
                return STATUS_SUCCESS;
            }

            NTSTATUS ReadByBytes(__out LONG64 & value) noexcept
            {
                byte numArray[sizeof(LONG64)];
                NTSTATUS status = memChannel_.Read(sizeof(LONG64), numArray);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                value = static_cast<LONG64>(static_cast<ULONG32>((static_cast<LONG32>(*numArray) |
                    static_cast<LONG32>(numArray[1]) << 8 |
                    static_cast<LONG32>(numArray[2]) << 16 |
                    static_cast<LONG32>(numArray[3]) << 24))) |
                    static_cast<LONG64>((static_cast<LONG32>(numArray[4]) |
                        static_cast<LONG32>(numArray[5]) << 8 |
                        static_cast<LONG32>(numArray[6]) << 16 |
                        static_cast<LONG32>(numArray[7]) << 24)) << 32;
                return STATUS_SUCCESS;
            }
#pragma endregion

#pragma region STRING TYPES
            void Read(__out std::wstring & value)
            {
                NTSTATUS status = STATUS_SUCCESS;

                ASSERT_IFNOT(value.empty(), "BinaryReader: Read string must be empty. It is {0}", value);
                ULONG32 length = VarInt::ReadUInt32(*this);
                if (length == 0)
                {
                    // Return empty string.
                    value = L"";
                    return;
                }

                // Create a buffer to read the UTF8 string in to and zero it out.
                // Buffer size is length + 1 (null terminator)
                CHAR* buffer = _newArray<CHAR>(BINARYREADER_TAG, allocator_, length + 1);
                if (buffer == nullptr)
                {
                    memChannel_.Log(STATUS_INSUFFICIENT_RESOURCES);
                    throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                }
                KFinally([&] {_deleteArray(buffer); });

                // Zero out the allocated memory. This also set the null terminator.
                RtlZeroMemory(buffer, length + 1);

                // Copy 1: Reading from the memchannel into the buffer.
                ULONG readBytes = 0;
                status = memChannel_.Read(length, buffer, &readBytes);
                THROW_ON_FAILURE(status);
                ASSERT_IFNOT(readBytes == length, "BinaryReader: Did not read full string. Read readBytes is {0} and length is {1}", readBytes, length);

                #ifdef PLATFORM_UNIX
                    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> convertor;
                #else
                    wstring_convert<codecvt_utf8_utf16<unsigned short>, unsigned short> convertor;
                #endif

                // Copy 2: Converting the buffer with string (null terminated char*) in to UTF16 encoded wstring
                // Note: #10258883: Try to avoid the copy. Before we used w_char for the convertor and can just call
                // from_bytes to assign the wstring. But it segfault on Linux while running test.
                value.clear();
                value.reserve(length + 1);
                for (auto const & ch : convertor.from_bytes(buffer))
                {
                    value.push_back(ch);
                }
            }

            void Read(
                __out KString::SPtr & value, 
                __in_opt Encoding encoding = Encoding::UTF8)
            {
                NTSTATUS status = STATUS_UNSUCCESSFUL;

                if (encoding == Encoding::UTF8)
                {
                    // This operation does two copies.
                    wstring utf16string;
                    Read(utf16string);

                    // Copy 3: Converting the buffer with string (null terminated char*)
                    status = KString::Create(value, allocator_, utf16string.c_str());
                    THROW_ON_FAILURE(status);
                    ASSERT_IFNOT(value->IsNullTerminated() == TRUE, "BinaryReader: Read value is not null terminated");
                    return;
                }

                ULONG32 length = VarInt::ReadUInt32(*this);
                if (length == 0)
                {
                    // Return empty string.
                    status = KString::Create(value, allocator_, L"");
                    THROW_ON_FAILURE(status);
                    return;
                }

                ULONG32 lengthInBytes = length * sizeof(WCHAR);

                // Create a buffer to read the UTF8 string in to and zero it out.
                // Buffer size is lengthInBytes + sizeof(WCHAR) (null terminator)
                WCHAR* buffer = _newArray<WCHAR>(BINARYREADER_TAG, allocator_, lengthInBytes + sizeof(WCHAR));
                if (buffer == nullptr)
                {
                    memChannel_.Log(STATUS_INSUFFICIENT_RESOURCES);
                    throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                }
                KFinally([&] {_deleteArray(buffer); });

                // Zero out the allocated memory. This also set the null terminator.
                RtlZeroMemory(buffer, lengthInBytes + sizeof(WCHAR));

                // Copy 1: Reading from the memchannel into the buffer.
                ULONG readBytes = 0;
                status = memChannel_.Read(lengthInBytes, buffer, &readBytes);
                THROW_ON_FAILURE(status);
                ASSERT_IFNOT(readBytes == lengthInBytes, "BinaryReader: Did not read full KString. Read readBytes is {0} and length is {1}", readBytes, lengthInBytes);

                // Copy 2: Converting the buffer with KString.
                status = KString::Create(value, allocator_, buffer);
                THROW_ON_FAILURE(status);
            }

            void Read(__out KUri::SPtr & value)
            {
                NTSTATUS status = STATUS_UNSUCCESSFUL;

                // This operation does two copies.
                wstring utf16string;
                Read(utf16string);

                // Copy 3: Converting the buffer with KUri (null terminated char*)
                status = KUri::Create(utf16string.c_str(), allocator_, value);
                THROW_ON_FAILURE(status);
            }

            void Read(__out KStringA::SPtr & value)
            {
                NTSTATUS status = STATUS_SUCCESS;

                ULONG32 length = VarInt::ReadUInt32(*this);
                if (length == 0)
                {
                    // Return empty string.
                    KStringA::SPtr output;
                    status = KStringA::Create(output, allocator_, "");
                    THROW_ON_FAILURE(status);

                    value = output;
                    return;
                }

                // Create a buffer to read the UTF8 string in to and zero it out.
                // Buffer size is length + 1 (null terminator)
                CHAR* buffer = _newArray<CHAR>(BINARYREADER_TAG, allocator_, length + 1);
                if (buffer == nullptr)
                {
                    memChannel_.Log(STATUS_INSUFFICIENT_RESOURCES);
                    throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                }
                KFinally([&] {_deleteArray(buffer); });

                // Zero out the allocated memory. This also set the null terminator.
                RtlZeroMemory(buffer, length + 1);

                // Copy 1: Reading from the memchannel into the buffer.
                ULONG readBytes = 0;
                status = memChannel_.Read(length, buffer, &readBytes);
                THROW_ON_FAILURE(status);
                ASSERT_IFNOT(readBytes == length, "BinaryReader: Did not read full KStringA. Read readBytes is {0} and length is {1}", readBytes, length);

                KStringA::SPtr output;
                status = KStringA::Create(output, allocator_, buffer);
                THROW_ON_FAILURE(status);

                value = output;
            }
#pragma endregion

#pragma region BUFFER TYPES
            void Read(__out KBuffer::SPtr & value);

            void Read(
                __in ULONG sizeToRead, 
                __out KBuffer::SPtr & value)
            {
                ASSERT_IFNOT(sizeToRead > 0, "Invalid read size: {0} in binary reader", sizeToRead);

                NTSTATUS status = memChannel_.Read(sizeToRead, value->GetBuffer());
                THROW_ON_FAILURE(status);
            }
#pragma endregion

        private:

            BinaryReader();

            // Adopted from KSerial.h and additionally throws exception on failures due to allocations or reads
            void Read(__out PUNICODE_STRING unicodeString)
            {
                ULONG32 length = VarInt::ReadUInt32(*this);

                if (length)
                {
                    unicodeString->MaximumLength = static_cast<USHORT>((length * 2) + 2);
                    unicodeString->Length = static_cast<USHORT>(length * 2);

                    unicodeString->Buffer = PWSTR(_newArray<UCHAR>(BINARYREADER_TAG, allocator_, (length * 2) + 2));
                    if (unicodeString->Buffer == nullptr)
                    {
                        memChannel_.Log(STATUS_INSUFFICIENT_RESOURCES);
                        throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                    }

                    RtlZeroMemory(unicodeString->Buffer, (length * 2) + 2);

                    for (ULONG32 index = 0; index < length; index++)
                    {
                        char myChar;
                        NTSTATUS status = memChannel_.Read(sizeof(char), &myChar);
                        THROW_ON_FAILURE(status);

                        unicodeString->Buffer[index] = myChar;
                    }
                }
                else
                {
                    unicodeString->Length = 0;
                    unicodeString->Buffer = nullptr;
                    unicodeString->MaximumLength = 0;
                }
            }

            KInChannel in_;
            KMemChannel memChannel_;
            KAllocator & allocator_;
        };
    }
}
