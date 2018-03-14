// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Serialization
{
    struct FabricTypeInformation
    {
        UCHAR const * buffer;
        ULONG length;
        static ULONG const LengthMax = 100 * 1024 * 1024; // 100 MB

        void Clear()
        {
            this->buffer = nullptr;
            this->length = 0;
        }
    };
}
