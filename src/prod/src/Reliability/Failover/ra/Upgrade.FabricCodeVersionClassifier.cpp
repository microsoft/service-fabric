// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Upgrade;
using namespace Infrastructure;

FabricCodeVersionProperties FabricCodeVersionClassifier::Classify(FabricCodeVersion const & version)
{
    static FabricCodeVersion const minimumVersionWithDeactivationInfoSupport(4, 0, 1402, 0);
    
    FabricCodeVersionProperties rv;
    
    if (version < minimumVersionWithDeactivationInfoSupport)
    {
        rv.IsDeactivationInfoSupported = false;
    }
    else
    {
        rv.IsDeactivationInfoSupported = true;
    }

    return rv;
}
