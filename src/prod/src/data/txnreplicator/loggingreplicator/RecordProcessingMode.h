// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // Indicates if the transaction is active or not
        //
        namespace RecordProcessingMode
        {
            enum Enum
            {
                Normal = 0,

                ApplyImmediately = 1,

                ProcessImmediately = 2
            };
        }
    }
}
