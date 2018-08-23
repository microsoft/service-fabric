//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

namespace ServiceModel
{
    namespace SingleInstanceDeploymentStatus
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Enum::Invalid: w << "Invalid"; return;
                case Enum::Creating: w << "Creating"; return;
                case Enum::Ready: w << "Ready"; return;
                case Enum::Deleting: w << "Deleting"; return;
                case Enum::Failed: w << "Failed"; return;
                case Enum::Upgrading: w << "Upgrading"; return;
            }
            w << "SingleInstanceDeploymentStatus({0}" << static_cast<int>(e) << ')';
        }
    }
}
