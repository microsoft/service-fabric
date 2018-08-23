// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace HealthManager
    {
        namespace RequestContextKind 
        {
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e)
            {
                switch(e)
                {
                    case Report: w << "Report"; return;
                    case SequenceStream: w << "SequenceStream"; return;
                    case QueryEntityDetail: w << "QueryEntityDetail"; return;
                    case QueryEntityChildren: w << "QueryEntityChildren"; return;
                    case Query: w << "Query"; return;
                    case QueryEntityHealthStateChunk: w << "QueryEntityHealthStateChunk"; return;
                    case QueryEntityHealthState: w << "QueryEntityHealthState"; return;
                    case QueryEntityUnhealthyChildren: w << "QueryEntityUnhealthyChildren"; return;
                    default: w << "RequestContextKind(" << static_cast<uint>(e) << ')'; return;
                }
            }

            BEGIN_ENUM_STRUCTURED_TRACE( RequestContextKind )

            ADD_CASTED_ENUM_MAP_VALUE_RANGE( RequestContextKind, Report, LAST_STATE)

            END_ENUM_STRUCTURED_TRACE( RequestContextKind )
        }
    }
}
