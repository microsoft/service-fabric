// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace ServiceModel;

namespace Management
{
    namespace HealthManager
    {
        namespace HealthJobItemKind 
        {
            void WriteToTextWriter(__in Common::TextWriter & w, Enum e)
            {
                switch(e)
                {
                case Unknown: w << "Unknown"; return;
                case ProcessReport: w << "ProcessReport"; return;
                case DeleteEntity: w << "DeleteEntity"; return;
                case CleanupEntity: w << "CleanupEntity"; return;
                case UpdateEntityEvents: w << "UpdateEntityEvents"; return;
                case ProcessSequenceStream: w << "ProcessSequenceStream"; return;
                case CheckInMemoryEntityData: w << "CheckInMemoryEntityData"; return;
                default: w << "HealthJobItemKind(" << static_cast<uint>(e) << ')'; return;
                }
            }

            std::wstring const & GetTypeString(HealthJobItemKind::Enum kind)
            {
                switch(kind)
                {
                case ProcessReport: return Constants::ProcessReportJobItemId;
                case DeleteEntity: return Constants::DeleteEntityJobItemId;
                case CleanupEntity: return Constants::CleanupEntityJobItemId;
                case UpdateEntityEvents: return Constants::CleanupEntityExpiredTransientEventsJobItemId;
                case ProcessSequenceStream: return Constants::ProcessSequenceStreamJobItemId;
                case CheckInMemoryEntityData: return Constants::CheckInMemoryEntityDataJobItemId;
                default: Common::Assert::CodingError("unsupported job item kind {0}", kind);
                }
            }

            BEGIN_ENUM_STRUCTURED_TRACE( HealthJobItemKind )

            ADD_CASTED_ENUM_MAP_VALUE_RANGE( HealthJobItemKind, Unknown, LAST_STATE)

            END_ENUM_STRUCTURED_TRACE( HealthJobItemKind )
        }
    }
}
