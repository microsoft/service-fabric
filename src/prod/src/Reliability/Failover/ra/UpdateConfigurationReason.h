// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace UpdateConfigurationReason
        {
            enum Enum
            {
                // UC is being called prior to catchup as part of an action list that involves catchup
                Catchup,

                // UC is being called as part of an action list that is performed during end reconfig
                Current,

                // UC is being called prior to revoking write status during swap
                PreWriteStatusRevokeCatchup,

                // UC is being called from any other action list or such stuff
                Default
            };
        }
    }
}

