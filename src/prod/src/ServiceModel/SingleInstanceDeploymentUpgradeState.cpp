//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace ServiceModel
{
    namespace SingleInstanceDeploymentUpgradeState
    {
        void WriteToTextWriter(TextWriter & w, Enum const & e)
        {
            switch (e)
            {
                case Enum::Invalid: w << "Invalid"; return;
                case Enum::ProvisioningTarget: w << "ProvisioningTarget"; return;
                case Enum::RollingForward: w << "RollingForward"; return;
                case Enum::UnprovisioningCurrent: w << "UnprovisioningCurrent"; return;
                case Enum::CompletedRollforward: w << "CompletedRollforward"; return;
                case Enum::RollingBack: w << "RollingBack"; return;
                case Enum::UnprovisioningTarget: w << "UnprovisioningTarget"; return;
                case Enum::CompletedRollback: w << "CompletedRollback"; return;
                case Enum::Failed: w << "Failed"; return;
            };

            w << "SingleInstanceDeploymentUpgradeState(" << static_cast<int>(e) << ')';
        }
    }
}
