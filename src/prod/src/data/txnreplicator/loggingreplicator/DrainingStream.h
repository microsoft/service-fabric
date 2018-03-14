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
        // Draining stream
        //
        namespace DrainingStream 
        {
            enum Enum
            {
                Invalid = 0,

                // As the name implies, this is during recovery
                Recovery,

                // This is during full copy on an idle in the state stream
                State,

                // This is during copy on an idle in the copy stream
                Copy,

                // This is on an active secondary replication stream
                Replication,

                Primary,

                LastValidEnum = Primary 
            };

            void WriteToTextWriter(Common::TextWriter & w, Enum const & val);

            DECLARE_ENUM_STRUCTURED_TRACE(DrainingStream);
        }
    }
}
