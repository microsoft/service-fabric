// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        //
        // A Binary Writer abstraction for writing objects into a memory stream
        // Current implementation uses a KOutChannel to write the data into a memory channel
        //
        class BinaryWriter 
            : public KObject<BinaryWriter>
        {

        public:

            BinaryWriter(__in KAllocator & allocator);

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
                ULONG size = memChannel_.Size();

                if (size < value)
                {
                    // Calculate the difference between the current memChannel size and our target position
                    ULONG diff = value - size;
                    char dummy = '\0';
                    NTSTATUS status = STATUS_SUCCESS;

                    // #11444928: Throwing coding error - BinaryWriter : set_Position memchannel cursor 95 is not equal to value 96
                    // Ensure the cursor position is equal to the last valid position in the memChannel
                    // This is necessary to ensure the below padding results in memChannel cursor == value 
                    // Otherwise, final cursor position might not be equal to target position despite padding

                    status = memChannel_.SetCursor(size, KMemChannel::eFromBegin);

                    ASSERT_IFNOT(
                        NT_SUCCESS(status),
                        "BinaryWriter:set_Position: KMemChannel failed to set cursor. Actual: {0}. Target:{1}",
                        memChannel_.GetCursor(),
                        value);

                    while (diff > 0)
                    {
                        status = memChannel_.Write(sizeof(dummy), &dummy);

                        ASSERT_IFNOT(
                            NT_SUCCESS(status),
                            "BinaryWriter:set_Position: KMemChannel failed to write. Actual: {0}. Target:{1}. Current diff:{2}",
                            memChannel_.GetCursor(),
                            value,
                            diff);

                        diff--;
                    }
                }
                else
                {
                    NTSTATUS status = memChannel_.SetCursor(value, KMemChannel::eFromBegin);
                    ASSERT_IFNOT(
                        NT_SUCCESS(status),
                        "BinaryWriter:set_Position: KMemChannel failed to set cursor. Actual: {0}. Target:{1}",
                        memChannel_.GetCursor(),
                        value);
                }

                ASSERT_IFNOT(
                    memChannel_.GetCursor() == value,
                    "BinaryWriter:set_Position: KMemChannel cursor {0} is not equal to value {1}",
                    memChannel_.GetCursor(),
                    value);
            }

            //
            // Gets the size of the underlying KMemChannel. 
            // TEST ONLY
            //
            __declspec(property(get = get_TestSize)) ULONG Test_Size;
            ULONG get_TestSize() const
            {
                return memChannel_.Size();
            }

            //
            //  Releases the underlying memory and brings it back to the original state when just constructed
            //
            void Reset();

#pragma region POD BASED TYPES
            void Write(__in bool value)
            {
                NTSTATUS status = memChannel_.Write(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Write(__in byte value)
            {
                NTSTATUS status = memChannel_.Write(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Write(__in ULONG32 value)
            {
                NTSTATUS status = memChannel_.Write(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Write(__in LONG32 value)
            {
                NTSTATUS status = memChannel_.Write(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Write(__in ULONG64 value)
            {
                NTSTATUS status = memChannel_.Write(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Write(__in LONG64 value)
            {
                NTSTATUS status = memChannel_.Write(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }

            void Write(__in KGuid const & value)
            {
                NTSTATUS status = memChannel_.Write(sizeof(value), &value);
                THROW_ON_FAILURE(status);
            }
#pragma endregion

#pragma region
            // Note: In managed NamedOperationDataCollection, we used BitConverter::GetBytes to turn int
            // into bytes and serialize it. To make the copy part backwards compatible, we write the same thing.
            // LONG32 is used for writing version and LONG64 is used for writing stateProviderId
            NTSTATUS WriteInBytes(__in LONG32 value)
            {
                byte numArray[sizeof(LONG32)];
                byte * numPtr = numArray;
                *(LONG32 *)numPtr = value;
                return memChannel_.Write(sizeof(LONG32), numArray);
            }

            NTSTATUS WriteInBytes(__in LONG64 value)
            {
                byte numArray[sizeof(LONG64)];
                byte * numPtr = numArray;
                *(LONG64 *)numPtr = value;
                return memChannel_.Write(sizeof(LONG64), numArray);
            }
#pragma endregion

#pragma region STRING TYPES
            void Write(
                __in KString const & value, 
                __in_opt Encoding encoding = Encoding::UTF8)
            {
                Write(static_cast<KStringView>(value), encoding);
            }

            void Write(
                __in KStringView const & value, 
                __in_opt Encoding encoding  = Encoding::UTF8)
            {
                NTSTATUS status = STATUS_UNSUCCESSFUL;

                ASSERT_IFNOT(value.IsNullTerminated(), "BinaryWriter: Write KStringView value is not null terminated");

                if (encoding == Encoding::UTF8)
                {
#ifdef PLATFORM_UNIX
                    wstring_convert<codecvt_utf8_utf16<char16_t>, char16_t> convertor;
                    std::string utf8String = convertor.to_bytes((const char16_t *)static_cast<LPCWSTR>(value));
#else
                    wstring_convert<codecvt_utf8_utf16<unsigned short>, unsigned short> convertor;
                    std::string utf8String = convertor.to_bytes((const unsigned short *)static_cast<LPCWSTR>(value));
#endif

                    Write(utf8String);
                    return;
                }

                VarInt::Write(*this, static_cast<ULONG32>(value.Length()));
                status = memChannel_.Write(static_cast<ULONG32>(value.LengthInBytes()), value);
                THROW_ON_FAILURE(status);
            }

            void Write(__in std::string const & value)
            { 
                size_t length = value.length();
                ASSERT_IF(length > ULONG_MAX, "BinaryWriter: Write std::string value is larger than ULONG MAX {0}", length);

                VarInt::Write(*this, static_cast<ULONG32>(length));
                NTSTATUS status = memChannel_.Write(static_cast<ULONG32>(length), value.c_str());
                THROW_ON_FAILURE(status);
            }

            void Write(__in KUri const & value)
            {
                Write(value.Get(KUriView::UriFieldType::eRaw));
            }

            void Write(__in KStringA const & value)
            {
                ASSERT_IFNOT(value.IsNullTerminated(), "BinaryWriter: Write KStringA value is not null terminated");

                VarInt::Write(*this, static_cast<ULONG32>(value.Length()));
                NTSTATUS status = memChannel_.Write(static_cast<ULONG32>(value.Length()), value);
                THROW_ON_FAILURE(status);
            }

            void Write(__in KStringViewA const & value)
            {
                ASSERT_IFNOT(value.IsNullTerminated(), "BinaryWriter: Write KStringViewA value is not null terminated");

                VarInt::Write(*this, static_cast<ULONG32>(value.Length()));
                NTSTATUS status = memChannel_.Write(static_cast<ULONG32>(value.Length()), value);
                THROW_ON_FAILURE(status);
            }
#pragma endregion

#pragma region BUFFER TYPES
            void Write(__in_opt KBuffer const * const value);

            void Write(
                __in_opt KBuffer const * const value, 
                __in ULONG length)
            {
                ASSERT_IF(value == nullptr, "BinaryWriter: Write KBuffer is null");
                ASSERT_IF(value->QuerySize() < length, "BinaryWriter: Write KBuffer size {0} is smaller than the length requested {1}", value->QuerySize(), length);

                NTSTATUS status = memChannel_.Write(length, value->GetBuffer());
                THROW_ON_FAILURE(status);
            }

            void Write(__in KBuffer& value)
            {
                ULONG length = value.QuerySize();
                NTSTATUS status = memChannel_.Write(length, value.GetBuffer());
                THROW_ON_FAILURE(status);
            }
#pragma endregion

            // 
            // TODO: Remove COPY
            // Returns a copy of the underlying buffer from the specified start position to the current position
            //
            KBuffer::SPtr GetBuffer(__in ULONG startPosition);

            // 
            // TODO: Remove COPY
            // Returns a copy of the underlying buffer from the specified start position
            //
            KBuffer::SPtr GetBuffer(
                __in ULONG startPosition,
                __in ULONG numberOfBytesToRead);

        private:

            BinaryWriter();

            KMemChannel memChannel_;
            KAllocator & allocator_;
        };
    }
}
