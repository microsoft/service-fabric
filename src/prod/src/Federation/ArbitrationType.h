// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    namespace ArbitrationType
    {
        enum Enum
        {
            TwoWaySimple = 0,
            TwoWayExtended = 1,
            OneWay = 2,
            RevertToReject = 3,
            RevertToNeutral = 4,
            Implicit = 5,
            Query = 6,
            KeepAlive = 7
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    }
}

