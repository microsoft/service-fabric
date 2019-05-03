// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace RepairPolicyEngine
{
    Common::Global<RepairPolicyEngineEventSource> RepairPolicyEngineEventSource::Trace = 
        Common::make_global<RepairPolicyEngineEventSource>();
}
