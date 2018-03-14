// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    namespace RepairCondition
    {
        enum Enum
        {
            None = 0,
            CorruptDatabaseLogs = 1,
            DatabaseFilesAccessDenied = 2,

            FirstValidEnum = None,
            LastValidEnum = DatabaseFilesAccessDenied
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & e);

        DECLARE_ENUM_STRUCTURED_TRACE( RepairCondition )
    };
}
