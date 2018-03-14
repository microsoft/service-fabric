// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    namespace TcpConnectionState
    {
        enum Enum
        {
            None = 0,
            Created = 1,
            Connecting = 2,
            Connected = 3,
            CloseDraining = 4,
            Closed = 5,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(TcpConnectionState)
    }
}
