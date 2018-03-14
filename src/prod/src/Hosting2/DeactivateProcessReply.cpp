// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DeactivateProcessReply::DeactivateProcessReply() 
    : appServiceId_(),
    error_(ErrorCodeValue::Success)
{
}

DeactivateProcessReply::DeactivateProcessReply(
    std::wstring const & applicationServiceId,
    ErrorCode error)
    : appServiceId_(applicationServiceId),
    error_(error)
{
}

void DeactivateProcessReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DeactivateProcessReply { ");
    w.Write("ApplicationServiceId = {0}", appServiceId_);
    w.Write("Error = {0}", error_);
    w.Write("}");
}
