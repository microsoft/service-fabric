// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerOperationReply::ContainerOperationReply()
    : error_()
    , errorMessage_()
{
}

ContainerOperationReply::ContainerOperationReply(
    ErrorCode const & error,
    wstring const & errorMessage)
    : error_(error)
    , errorMessage_(errorMessage)
{
}

wstring && ContainerOperationReply::TakeErrorMessage()
{
    return move(errorMessage_);
}

void ContainerOperationReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerOperationReply { ");
    w.Write("Error = {0}", error_);
    w.Write("ErrorMessage = {0}", errorMessage_);
    w.Write("}");
}
