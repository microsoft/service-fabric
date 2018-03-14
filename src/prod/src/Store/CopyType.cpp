// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Store
{
    namespace CopyType
    {

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e)
        {			
            switch (e)
            {
            case PagedCopy: w << "PagedCopy"; return;
            case FirstFullCopy: w << "FirstFullCopy"; return;
            case FirstPartialCopy: w << "FirstPartialCopy"; return;
            case FirstSnapshotPartialCopy: w << "FirstSnapshotPartialCopy"; return;
            case FileStreamFullCopy: w << "FileStreamFullCopy"; return;
            case FileStreamRebuildCopy: w << "FileStreamRebuildCopy"; return;

            default: 
                w << "CopyType(" << static_cast<ULONG>(e) << ')';
            }			
        }

        ENUM_STRUCTURED_TRACE(CopyType, FirstValidEnum, LastValidEnum)
    }
}
