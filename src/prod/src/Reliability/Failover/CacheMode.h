// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace CacheMode
    {
        enum Enum
        {
            UseCached = 0,
            Refresh = 1,
            BypassCache = 2,
        };
        
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
