// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServiceTerminatedNotification::ServiceTerminatedNotification()
    : activityDescription_(Common::ActivityDescription::Empty),
    parentId_(),
    appServiceIds_(),
    exitCode_(),
    isContainerRoot_(false)
{
}

ServiceTerminatedNotification::ServiceTerminatedNotification(
    Common::ActivityDescription const & activityDescription,
    wstring const & parentId,
    vector<wstring> const & appServiceIds,
    DWORD exitCode,
    bool isContainerGroup)
    : activityDescription_(activityDescription),
    parentId_(parentId), 
    appServiceIds_(appServiceIds),
    exitCode_(exitCode),
    isContainerRoot_(isContainerGroup)
{
}

void ServiceTerminatedNotification::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceTerminatedNotification { ");
    w.Write("ActivityDescription = {0}", activityDescription_);
    w.Write("ParentId = {0}", parentId_);
    for (auto it = appServiceIds_.begin(); it != appServiceIds_.end(); ++it)
    {
        w.Write("AppServiceId = {0}", *it);
    }
    w.Write("ExitCode = {0}", exitCode_);
    w.Write("IsContainerGroup = {0}", isContainerRoot_);
    w.Write("}");
}
