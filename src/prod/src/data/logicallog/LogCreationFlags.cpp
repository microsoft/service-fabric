// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Data
{
    namespace Log
    {
        void WriteToTextWriter(Common::TextWriter & w, LogCreationFlags const & e)
        {
            switch (e)
            {
            case LogCreationFlags::UseNonSparseFile: w << "UseNonSparseFile"; return;
            case LogCreationFlags::UseSparseFile: w << "UseSparseFile"; return;
            }
            w << "LogCreationFlags(" << static_cast<ULONG>(e) << ')';
        }
    }
}
