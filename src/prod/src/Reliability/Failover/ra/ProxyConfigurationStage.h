// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ProxyConfigurationStage
        {
            enum Enum
            {
                Current = 0,
                CurrentPending = 1,
                Catchup = 2,
                CatchupPending = 3,
                PreWriteStatusRevokeCatchup = 4,
                PreWriteStatusRevokeCatchupPending = 5,
                LastValidEnum = PreWriteStatusRevokeCatchupPending
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(ProxyConfigurationStage);
        }
    }
}
