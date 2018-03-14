// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Transport
{
    namespace TransportPriority
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Normal: w << "Normal"; return;
                case High: w << "High"; return;
            }
            w << "TransportPriority(" << static_cast<uint>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE( TransportPriority, Normal, High )
    }
}
