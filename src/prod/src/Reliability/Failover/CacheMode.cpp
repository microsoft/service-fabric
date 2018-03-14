// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Failover.stdafx.h"

namespace Reliability
{
    namespace CacheMode
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case UseCached: w << "UseCached"; return;
            case Refresh: w << "Refresh"; return;
            case BypassCache: w << "BypassCache"; return;
            }

            w << "CacheMode(" << static_cast<int>(e) << ')';
        }
     }
}
