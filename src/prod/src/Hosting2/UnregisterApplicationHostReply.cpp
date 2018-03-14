// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

UnregisterApplicationHostReply::UnregisterApplicationHostReply()
    : errorCode_()
{
}

UnregisterApplicationHostReply::UnregisterApplicationHostReply(ErrorCode const & errorCode)
    : errorCode_(errorCode)
{
}

void UnregisterApplicationHostReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UnregisterApplicationHostReply { ");
    w.Write("Error = {0}", Error);
    w.Write("}");
}
