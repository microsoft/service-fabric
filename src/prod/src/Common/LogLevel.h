// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace LogLevel
    {
        enum Enum
        {
            Silent   = 0,
            Critical = 1,
            Error    = 2,
            Warning  = 3,
            Info     = 4,
            Noise    = 5,

            All     = 99
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
