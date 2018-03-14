// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace TraceChannelType
    {
        enum Enum
        {
            Admin        = 0,
            Operational  = 1,
            Analytic     = 2,
            Debug        = 3,
            DeprecatedOperational  = 4
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);
    }
}
