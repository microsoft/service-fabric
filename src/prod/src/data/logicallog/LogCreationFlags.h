// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Log
    {
        enum class LogCreationFlags : ULONG
        {
            UseNonSparseFile = 0,
            UseSparseFile = 1
        };

        void WriteToTextWriter(Common::TextWriter & w, LogCreationFlags const & e);
    }
}
