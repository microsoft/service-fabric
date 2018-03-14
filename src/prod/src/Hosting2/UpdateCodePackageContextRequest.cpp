// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

UpdateCodePackageContextRequest::UpdateCodePackageContextRequest()
    : codeContext_(),
    activationId_(),
    timeout_()
{
}

UpdateCodePackageContextRequest::UpdateCodePackageContextRequest(
    CodePackageContext const & codeContext, 
    CodePackageActivationId const & activationId,
    TimeSpan const timeout)
    : codeContext_(codeContext),
    activationId_(activationId),
    timeout_(timeout)
{
}

void UpdateCodePackageContextRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("UpdateCodePackageContextRequest { ");
    w.Write("CodeContext = {0}, ", CodeContext);
    w.Write("ActivationId = {0}, ", ActivationId);
    w.Write("Timeout = {0}, ", Timeout);
    w.Write("}");
}
