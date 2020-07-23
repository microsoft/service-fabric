// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ApplicationHostEvent.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;

ApplicationHostEvent::ApplicationHostEvent(
    Hosting2::CodePackageInstanceIdentifier const & codePackageInstanceIdentifier,
    std::wstring const & appName,
    ServiceModel::EntryPointType::Enum hostType,
    std::wstring const & appHostId,
    bool isUp,
    bool isLinuxContainerIsolated)
    : codePackageInstanceIdentifier_(codePackageInstanceIdentifier),
    appName_(appName),
    hostType_(hostType),
    appHostId_(appHostId),
    isUp_(isUp),
    isLinuxContainerIsolated_(isLinuxContainerIsolated)
{
}

void ApplicationHostEvent::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationHostEvent { ");
    w.Write("CodePackageInstanceIdentifier = {0}", codePackageInstanceIdentifier_);
    w.Write("ApplicationName = {0}", appName_);
    w.Write("MetadataInformation = {0}", appHostId_);
    w.Write("HostType = {0}", hostType_);
    w.Write("IsUp = {0}", isUp_);
    w.Write("IsLinuxContainerIsolated = {0}", isLinuxContainerIsolated_);
    w.Write("}");
}
