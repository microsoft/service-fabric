// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"
#include "../TestApi.Failover.Internal.h"

using namespace std;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace ReliabilityTestApi;
using namespace ReconfigurationAgentComponent;
using namespace ReconfigurationAgentComponentTestApi;
using namespace Infrastructure;

ReconfigurationAgentProxyTestHelper::ReconfigurationAgentProxyTestHelper(Reliability::ReconfigurationAgentComponent::IReconfigurationAgentProxy & rap) :
    rap_(dynamic_cast<ReconfigurationAgentProxy&>(rap))
{
}

vector<FailoverUnitId> ReconfigurationAgentProxyTestHelper::GetItemsInLFUPM()
{
    return rap_.LocalFailoverUnitProxyMapObj.GetAllIds();
}
