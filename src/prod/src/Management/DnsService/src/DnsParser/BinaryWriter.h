// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    class BinaryWriter
    {
        K_NO_DYNAMIC_ALLOCATE();
        K_DENY_COPY(BinaryWriter);

    public:
        BinaryWriter(
            __in KBuffer& buffer
        );

        ~BinaryWriter();

    public:
        bool WriteByte(__in char value);
        bool WriteUInt16(__in USHORT value);
        bool WriteUInt32(__in ULONG value);
        bool WriteBytes(
            __in_bcount(bufferSize) PVOID pBuffer,
            __in ULONG bufferSize,
            __in ULONG writeLength
        );

        ULONG GetWritePosition() const { return _writePosition; }

    private:
        template<class T>
        bool Write(
            __in T value
        )
        {
            if (GetWritePosition() + sizeof(T) > Size())
            {
                return false;
            }

            memcpy_s(GetWriteBuffer(), FreeSpace(), reinterpret_cast<PVOID>(&value), sizeof(T));
            IncrementWritePositionBy(sizeof(T));

            return true;
        }

        char* GetBuffer() { return static_cast<char*>(_buffer.GetBuffer()); }

        char* GetWriteBuffer() { return static_cast<char*>(GetBuffer() + _writePosition); }

        ULONG Size() const { return _buffer.QuerySize(); }

        ULONG FreeSpace() const { return Size() - GetWritePosition(); }

        void IncrementWritePositionBy(
            __in ULONG value
        );

    private:
        KBuffer& _buffer;
        ULONG _writePosition;
    };
}

