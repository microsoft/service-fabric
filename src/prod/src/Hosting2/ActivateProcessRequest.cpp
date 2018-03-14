// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ActivateProcessRequest::ActivateProcessRequest() 
    : parentProcessId_(),
    nodeId_(),
    applicationId_(),
    appServiceId_(),
    processDescription_(),
    securityUserId_(),
    ticks_(),
    fabricBinFolder_(),
    isContainerHost_(false),
    containerDescription_()
{
}

ActivateProcessRequest::ActivateProcessRequest(
    DWORD parentProcessId,
    wstring const & nodeId,
    wstring const & applicationId,
    wstring const & appServiceId,
    ProcessDescription const & processDescription,
    wstring const & userId,
    int64 ticks,
    wstring const & fabricBinFolder,
    bool isContainerHost,
    ContainerDescription const & containerDescription)
    : parentProcessId_(parentProcessId), 
    nodeId_(nodeId),
    applicationId_(applicationId),
    appServiceId_(appServiceId),
    processDescription_(processDescription),
    securityUserId_(userId),
    ticks_(ticks),
    fabricBinFolder_(fabricBinFolder),
    isContainerHost_(isContainerHost),
    containerDescription_(containerDescription)
{
}

void ActivateProcessRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateProcessRequest { ");
    w.Write("ParentProcessId = {0}", parentProcessId_);
    w.Write("NodeId = {0}", nodeId_);
    w.Write("ApplicationId = {0}", applicationId_);
    w.Write("AppServiceId = {0}", appServiceId_);
    w.Write("ProcessDescription = {0}", processDescription_);
    w.Write("UserId = {0}", securityUserId_);
    w.Write("TimeoutTicks = {0}", ticks_);
    w.Write("IsContainerHost = {0}", isContainerHost_);
    w.Write("ContainerDescription = {0}", containerDescription_);
    w.Write("}");
}
