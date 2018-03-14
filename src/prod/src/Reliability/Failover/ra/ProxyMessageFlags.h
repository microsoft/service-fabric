// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ProxyMessageFlags
        {
            enum Enum
            {
                None = 0x00,
                EndReconfiguration = 0x01,
                Catchup = 0x02,
                Abort = 0x04,
                Drop = 0x8,
                LastValidEnum = Drop
            };

            DECLARE_ENUM_STRUCTURED_TRACE(ProxyMessageFlags);

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
        };
    }
}
