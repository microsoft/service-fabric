// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    namespace TcpConnectionState
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case None: w << "None"; return;
                case Created: w << "Created"; return;
                case Connecting: w << "Connecting"; return;
                case Connected: w << "Connected"; return;
                case CloseDraining: w << "CloseDraining"; return;
                case Closed: w << "Closed"; return;
            }

            w << "TcpConnectionState(" << static_cast<int>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE(TcpConnectionState, None, Closed);
    }
}
