// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Upgrade
        {
            namespace UpgradeCancelResult
            {
                enum Enum
                {
                    Success = 0,
                    Queued = 1,
                    NotAllowed = 2,
                };
            }
        }
    }
}
