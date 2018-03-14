// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        namespace LoggerEngine
        {
            enum Enum
            {
                //Invalid
                Invalid = 0,

                //
                // The underlying log is a ktl logger
                //
                Ktl = 1,

                //
                // The underlying log is a file
                //
                File = 2,

                //
                // The underlying log is a memory stream
                //
                Memory = 2
            };
        }
    }
}
