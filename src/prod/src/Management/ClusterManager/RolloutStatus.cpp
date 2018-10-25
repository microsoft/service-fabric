// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ClusterManager
    {
        namespace RolloutStatus
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Unknown: w << "Unknown"; return;
                    case Pending: w << "Pending"; return;
                    case DeletePending: w << "DeletePending"; return;
                    case Failed: w << "Failed"; return;
                    case Completed: w << "Completed"; return;
                    case Replacing: w << "Replacing"; return;
                };

                w << "RolloutStatus(" << static_cast<int>(e) << ')';
            }
        }
    }
}

