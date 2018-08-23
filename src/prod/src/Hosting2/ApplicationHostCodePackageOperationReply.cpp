// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ApplicationHostCodePackageOperationReply::ApplicationHostCodePackageOperationReply()
    : error_()
{
}

ApplicationHostCodePackageOperationReply::ApplicationHostCodePackageOperationReply(ErrorCode const & error)
    : error_(error)
{
}

void ApplicationHostCodePackageOperationReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ApplicationHostCodePackageOperationReply { ");
    w.Write("Error = {0}", error_);
    w.Write("}");
}
