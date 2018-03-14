// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace RepairManager
    {
        namespace RepairManagerState
        {
            BEGIN_ENUM_TO_TEXT( RepairManagerState )
                ADD_ENUM_TO_TEXT_VALUE ( Unknown )
                ADD_ENUM_TO_TEXT_VALUE ( Stopped )
                ADD_ENUM_TO_TEXT_VALUE ( Starting )
                ADD_ENUM_TO_TEXT_VALUE ( Started )
                ADD_ENUM_TO_TEXT_VALUE ( Stopping )
                ADD_ENUM_TO_TEXT_VALUE ( StoppingPendingRestart )
                ADD_ENUM_TO_TEXT_VALUE ( Closed )
            END_ENUM_TO_TEXT( RepairManagerState )

            ENUM_STRUCTURED_TRACE( RepairManagerState, FirstValidEnum, LastValidEnum )
        }
    }
}
