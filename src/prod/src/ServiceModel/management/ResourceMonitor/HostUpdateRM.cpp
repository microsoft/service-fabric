// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ApplicationHostEvent.h"
#include "HostUpdateRM.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;

GlobalWString HostUpdateRM::HostUpdateRMAction = make_global<wstring>(L"HostUpdateRMAction");

HostUpdateRM::HostUpdateRM(std::vector<ApplicationHostEvent> && events)
    : events_(move(events))
{
}
