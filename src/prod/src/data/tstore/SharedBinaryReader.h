// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        class SharedBinaryReader :
            public KShared<SharedBinaryReader>,
            public BinaryReader
        {
            K_FORCE_SHARED(SharedBinaryReader)

        public:
            static NTSTATUS
                Create(
                    __in KAllocator& Allocator,
                    __in KBuffer& buffer,
                    __out SharedBinaryReader::SPtr& Result);

        private:
            SharedBinaryReader(__in KBuffer& buffer);
        };
    }
}

