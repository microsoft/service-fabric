// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

UpdateCodePackageContextReply::UpdateCodePackageContextReply() 
    : errorCode_()
{
}

UpdateCodePackageContextReply::UpdateCodePackageContextReply(
    ErrorCode const & errorCode)
    : errorCode_(errorCode)
{
}

void UpdateCodePackageContextReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UpdateCodePackageContextReply { ");
    w.Write("ErrorCode = {0}", Error);
    w.Write("}");
}
