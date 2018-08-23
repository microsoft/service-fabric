// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicaLifeCycleState
    {
        enum Enum
        {
            Invalid = 0,
            Closing = 1,

            LastValidEnum = Closing
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE(ReplicaLifeCycleState);
    }
}

