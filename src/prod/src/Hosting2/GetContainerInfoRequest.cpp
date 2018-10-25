// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GetContainerInfoRequest::GetContainerInfoRequest()
    : nodeId_(),
    appServiceId_(),
    isServicePackageActivationModeExclusive_(),
    containerInfoType_(),
    containerInfoArgs_()
{
}

GetContainerInfoRequest::GetContainerInfoRequest(
    wstring const & nodeId,
    wstring const & appServiceId,
    bool isServicePackageActivationModeExclusive,
    wstring const & containerInfoType,
    wstring const & containerInfoArgs)
    : nodeId_(nodeId),
    appServiceId_(appServiceId),
    isServicePackageActivationModeExclusive_(isServicePackageActivationModeExclusive),
    containerInfoType_(containerInfoType),
    containerInfoArgs_(containerInfoArgs)
{
}

void GetContainerInfoRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("GetContainerInfoRequest { ");
    w.Write("NodeId = {0}", nodeId_);
    w.Write("AppServiceId = {0}", appServiceId_);
    w.Write("IsServicePackageActivationModeExclusive = {0}", isServicePackageActivationModeExclusive_);
    w.Write("ContainerInfoType = {0}", containerInfoType_);
    w.Write("ContainerInfoArgs = {0}", containerInfoArgs_);
    w.Write("}");
}
