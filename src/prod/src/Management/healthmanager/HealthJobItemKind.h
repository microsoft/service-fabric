// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        namespace HealthJobItemKind 
        {
            enum Enum 
            { 
                Unknown = 0, 
                ProcessReport = 1,
                DeleteEntity = 2,
                CleanupEntity = 3,
                UpdateEntityEvents = 4,
                ProcessSequenceStream = 5,
                CheckInMemoryEntityData = 6,
                LAST_STATE = CheckInMemoryEntityData,
            };

            void WriteToTextWriter(__in Common::TextWriter & w, Enum e);

            std::wstring const & GetTypeString(HealthJobItemKind::Enum kind);

            DECLARE_ENUM_STRUCTURED_TRACE( HealthJobItemKind )
        }
    }
}
