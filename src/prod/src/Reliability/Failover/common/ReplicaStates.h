// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicaStates
    {
        enum Enum
        {
            StandBy = 0,
            InBuild = 1,
            Ready = 2,
            Dropped = 3,
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        std::wstring ToString(Enum const & val);
    }
}
