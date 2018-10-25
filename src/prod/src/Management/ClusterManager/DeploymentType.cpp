//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

namespace Management
{
    namespace ClusterManager
    {
        namespace DeploymentType
        {
            void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
            {
                switch (e)
                {
                    case Enum::Invalid: w << "Invalid"; return;
                    case Enum::Compose: w << "Compose"; return;
                    case Enum::V2Application: w << "V2Application"; return;
                }

                w << "DeploymentType(" << static_cast<int>(e) << ')';
            }
        }
    }
}
