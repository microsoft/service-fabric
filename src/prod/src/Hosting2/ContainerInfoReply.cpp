// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ContainerInfoReply::ContainerInfoReply() 
: error_(),
  containerInfo_()
{
}

ContainerInfoReply::ContainerInfoReply(
    ErrorCode const & error,
    wstring const & containerInfo)
    : error_(error),
    containerInfo_(containerInfo)
{
}

void ContainerInfoReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ContainerInfoReply { ");
    w.Write("Error = {0}", error_);
    w.Write("ContainerInfo = {0}", containerInfo_);
    w.Write("}");
}
