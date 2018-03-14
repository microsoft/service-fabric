// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace RetryableErrorAction
        {
            enum Enum
            {
                None                    = 0x00,
                Drop                    = 0x01,
                ReportHealthWarning     = 0x20,
                ClearHealthReport       = 0x40,
                Restart                 = 0x80,
                ReportHealthError       = 0x100,
            };
        }
    }
}
