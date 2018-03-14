// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ActivateContainerReply::ActivateContainerReply()
    : error_()
    , errorMessage_()
    , containerId_()
{
}

ActivateContainerReply::ActivateContainerReply(
    ErrorCode const & error,
    wstring const & errorMessage,
    wstring const & containerId)
    : error_(error)
    , errorMessage_(errorMessage)
    , containerId_(containerId)
{
}

wstring && ActivateContainerReply::TakeErrorMessage()
{
    return move(errorMessage_);
}

void ActivateContainerReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateContainerReply { ");
    w.Write("Error = {0}", error_);
    w.Write("ErrorMesaage={0}", errorMessage_);
    w.Write("ContainerId = {0}", containerId_);
    w.Write("}");
}
