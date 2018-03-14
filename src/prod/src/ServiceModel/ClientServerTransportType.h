// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ClientServerTransportType 
    {
        enum Enum
        {
            UnknownTransport = 0,
            TcpTransport = 1,
            HttpTransport = 2,
            PassThroughTransport = 3,
        };

        bool IsValid(Enum value);
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
