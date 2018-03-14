// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    class BinaryReader
    {
        K_NO_DYNAMIC_ALLOCATE();
        K_DENY_COPY(BinaryReader);

    public:
        BinaryReader(
            __in KBuffer& buffer,
            __in ULONG size
        );

        BinaryReader(
            __in KBuffer& buffer,
            __in ULONG size,
            __in ULONG position
        );

        ~BinaryReader();

    public:
        bool PeekByte(__out char& value) const;
        bool ReadByte(__out char& value);
        bool ReadUInt16(__out USHORT& value);
        bool ReadUInt32(__out ULONG& value);
        bool ReadBytes(__out KBuffer& buffer, __in ULONG length);
        bool Skip(__in ULONG length);

        ULONG Size() const { return _size; }
        KBuffer& GetBuffer() { return _buffer; }
        const char* GetReadBuffer() const { return static_cast<char*>(_buffer.GetBuffer()) + _readPosition; }

    private:
        template<class T>
        bool Peek(
            __out T& value
        ) const
        {
            if (GetReadPosition() + sizeof(T) > Size())
            {
                return false;
            }

            value = *reinterpret_cast<const T*>(GetReadBuffer());

            return true;
        }

        template<class T>
        bool Read(
            __out T& value
        )
        {
            if (!Peek(value))
            {
                return false;
            }

            IncrementReadPositionBy(sizeof(T));

            return true;
        }

        ULONG GetReadPosition() const { return _readPosition; }

        void SetReadPosition(
            __in ULONG value
        );

        void IncrementReadPositionBy(
            __in ULONG value
        );

    private:
        KBuffer& _buffer;
        const ULONG _size;
        ULONG _readPosition;
    };
}
