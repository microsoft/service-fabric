// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;

using namespace Common;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;
using namespace Health;

std::wstring PartitionHealthEventDescriptor::GenerateReportDescriptionInternal(HealthContext const & c) const
{
    return wformatString("{0}.", c.NodeName);    
}
