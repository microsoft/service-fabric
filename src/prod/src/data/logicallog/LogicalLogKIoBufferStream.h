// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#pragma once

namespace Data
{
    namespace Log
    {
        class LogicalLogKIoBufferStream 
            : public KIoBufferStream
        {

        public:

            LogicalLogKIoBufferStream();

            LogicalLogKIoBufferStream(__in KIoBuffer& underlyingIoBuffer, __in ULONG initialOffset);

            BOOLEAN
            Reuse(KIoBuffer& IoBuffer, ULONG InitialOffset);

            __declspec(property(get = get_Length)) ULONG Length;
            ULONG get_Length() const { return underlyingIoBuffer_->QuerySize(); }

            __declspec(property(get = GetPosition)) ULONG Position;
            ULONG get_Position() const { return GetPosition(); }

        private:

            KIoBuffer::SPtr underlyingIoBuffer_;
        };
    }
}
