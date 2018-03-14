// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ClusterManager
    {
        namespace RolloutManagerState
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Unknown: w << "Unknown"; return;
                    case NotActive: w << "NotActive"; return;
                    case Recovery: w << "Recovery"; return;
                    case Active: w << "Active"; return;
                    case Closed: w << "Closed"; return;
                };

                w << "RolloutManagerState(" << static_cast<int>(e) << ')';
            }

            ENUM_STRUCTURED_TRACE( RolloutManagerState, FirstValidEnum, LastValidEnum )
        }
    }
}
