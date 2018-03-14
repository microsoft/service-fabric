// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Serialization
{
    struct FabricIOBuffer
    {
        FabricIOBuffer()
            : buffer(nullptr)
            , length(0)
        {
        }

        FabricIOBuffer(UCHAR * Buffer, ULONG Length)
            : buffer(Buffer)
            , length(Length)
        {
        }


        UCHAR * buffer;
        ULONG length;

        void Clear()
        {
            this->buffer = nullptr;
            this->length = 0;
        }
    };
}
