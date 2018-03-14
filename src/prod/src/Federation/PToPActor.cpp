// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Federation
{
    namespace PToPActor
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & enumValue)
        {
            switch (enumValue)
            {
            case Direct: w << "Direct"; return;
            case Federation: w << "Federation"; return;
            case Routing: w << "Routing"; return;
            case Broadcast: w << "Broadcast"; return;
            case UpperBound: w << "UpperBound"; return;

            default: w << "PToPActor(" << static_cast<int>(enumValue) << ')'; return;
            }
        }
    }
}
