// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        namespace RequestContextKind 
        {
            enum Enum 
            { 
                Report = 0,
                SequenceStream = 1,
                QueryEntityDetail = 2,
                QueryEntityChildren = 3,
                Query = 4,
                QueryEntityHealthStateChunk = 5,
                QueryEntityHealthState = 6,
                QueryEntityUnhealthyChildren = 7,
                LAST_STATE = QueryEntityUnhealthyChildren,
            };

            void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

            DECLARE_ENUM_STRUCTURED_TRACE( RequestContextKind )
        }
    }
}
