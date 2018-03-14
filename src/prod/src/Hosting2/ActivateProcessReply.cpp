// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ActivateProcessReply::ActivateProcessReply() 
    : appServiceId_(), 
    processId_(0),
    error_(ErrorCodeValue::Success),
    errorMessage_()
{
}

ActivateProcessReply::ActivateProcessReply(
    wstring const & appServiceId,
    DWORD processId,
    ErrorCode error,
    wstring const & errorMessage)
    : appServiceId_(appServiceId), 
    processId_(processId), 
    error_(error),
    errorMessage_(errorMessage)
{
}

void ActivateProcessReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateProcessReply { ");
    w.Write("AppServiceId = {0}", appServiceId_);
    w.Write("ProcessId = {0}", processId_);
    w.Write("Error = {0}", error_);
    w.Write("Error message = {0}", errorMessage_);
    w.Write("}");
}
