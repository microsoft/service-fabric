// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

RegisterContainerActivatorServiceReply::RegisterContainerActivatorServiceReply()
    : error_()
    , isContainerServicePresent_()
    , isContainerServiceManaged_()
    , eventSinceTime_()
{
}

RegisterContainerActivatorServiceReply::RegisterContainerActivatorServiceReply(
    ErrorCode error,
    bool isContainerServicePresent,
    bool isContainerServiceManaged,
    uint64 eventSinceTime)
    : error_(error)
    , isContainerServicePresent_(isContainerServicePresent)
    , isContainerServiceManaged_(isContainerServiceManaged)
    , eventSinceTime_(eventSinceTime)
{
}

void RegisterContainerActivatorServiceReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("RegisterContainerActivatorServiceReply { ");
    w.Write("Error = {0}", error_);
    w.Write("IsContainerServicePresent = {0}", isContainerServicePresent_);
    w.Write("IsContainerServiceManaged = {0}", isContainerServiceManaged_);
    w.Write("EventSinceTime = {0}", eventSinceTime_);
    w.Write("}");
}
