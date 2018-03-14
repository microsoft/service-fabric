// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ClusterManager
    {
        namespace FabricUpgradeState
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Invalid: w << "Invalid"; return;
                    case CompletedRollforward: w << "CompletedRollforward"; return;
                    case CompletedRollback: w << "CompletedRollback"; return;
                    case RollingForward: w << "RollingForward"; return;
                    case RollingBack: w << "RollingBack"; return;
                    case Interrupted: w << "Interrupted"; return;
                    case Failed: w << "Failed"; return;
                };

                w << "FabricUpgradeState(" << static_cast<int>(e) << ')';
            }

            ENUM_STRUCTURED_TRACE( FabricUpgradeState, FirstValidEnum, LastValidEnum )
        }
    }
}


