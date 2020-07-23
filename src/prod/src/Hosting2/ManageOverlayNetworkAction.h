// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    namespace ManageOverlayNetworkAction
    {
        enum Enum
        {
            Assign = 0,
            Unassign = 1
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        std::wstring ToString(Enum const & val);
    }
}
