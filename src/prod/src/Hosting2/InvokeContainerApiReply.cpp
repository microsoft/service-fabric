// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

InvokeContainerApiReply::InvokeContainerApiReply()
    : error_()
    , errorMessage_()
    , apiExecResponse_()
{
}

InvokeContainerApiReply::InvokeContainerApiReply(
    ErrorCode const & error,
    wstring const & errorMessage,
    ContainerApiExecutionResponse const & apiExecResponse)
    : error_(error)
    , errorMessage_(errorMessage)
    , apiExecResponse_(apiExecResponse)
{
}

void InvokeContainerApiReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateContainerReply { ");
    w.Write("Error = {0}", error_);
    w.Write("ErrorMessage = {0}", errorMessage_);
    w.Write("ApiExecResponse = {0}", apiExecResponse_);
    w.Write("}");
}
