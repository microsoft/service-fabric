// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    namespace NodePhase
    {
        // Structure represents information about a particular instance of a node.
        enum Enum
        {
            Booting = 0,
            Joining = 1,
            Inserting = 2,
            Routing = 3,
            Shutdown = 4,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    }
}

