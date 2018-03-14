// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ResourceMonitorServiceRegistration.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;

GlobalWString ResourceMonitorServiceRegistration::ResourceMonitorServiceRegistrationAction = make_global<wstring>(L"ResourceMonitorServiceRegistrationAction");

ResourceMonitorServiceRegistration::ResourceMonitorServiceRegistration(std::wstring const & hostId)
    : id_(hostId)
{
}

void ResourceMonitorServiceRegistration::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ResourceMonitorServiceRegistration { ");
    w.Write("HostId = {0}", id_);
    w.Write("}");
}
