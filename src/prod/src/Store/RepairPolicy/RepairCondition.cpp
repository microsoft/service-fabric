// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    namespace RepairCondition
    {
        void WriteToTextWriter(TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case None: w << "None"; return;
                case CorruptDatabaseLogs: w << "CorruptDatabaseLogs"; return;
                case DatabaseFilesAccessDenied: w << "DatabaseFilesAccessDenied"; return;
            }
            w << "RepairCondition(" << static_cast<uint>(e) << ')';
        }

        ENUM_STRUCTURED_TRACE( RepairCondition, FirstValidEnum, LastValidEnum )
    }
}

