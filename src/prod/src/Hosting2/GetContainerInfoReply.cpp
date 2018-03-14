// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GetContainerInfoReply::GetContainerInfoReply() 
    : error_(ErrorCodeValue::Success)
{
}

GetContainerInfoReply::GetContainerInfoReply(
    std::wstring const & containerInfo,
    ErrorCode error)
    : containerInfo_(containerInfo),
    error_(error)
{
    auto maxContainerInfoContentSize = static_cast<size_t>(
        ServiceModelConfig::GetConfig().MaxMessageSize * ServiceModelConfig::GetConfig().MessageContentBufferRatio);
    StringUtility::TruncateStartIfNeeded(containerInfo_, maxContainerInfoContentSize);
}

void GetContainerInfoReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("GetContainerInfoReply { ");
    w.Write("ContainerInfo = {0}", containerInfo_);
    w.Write("Error = {0}", error_);
    w.Write("}");
}
