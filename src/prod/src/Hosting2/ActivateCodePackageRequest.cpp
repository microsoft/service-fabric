// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

ActivateCodePackageRequest::ActivateCodePackageRequest()
    : codeContext_(),
    activationId_(),
    timeout_()
{
}

ActivateCodePackageRequest::ActivateCodePackageRequest(
    CodePackageContext const & codeContext, 
    CodePackageActivationId const & activationId,
    TimeSpan const timeout)
    : codeContext_(codeContext),
    activationId_(activationId),
    timeout_(timeout)
{
}

void ActivateCodePackageRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateCodePackageRequest { ");
    w.Write("CodeContext = {0}, ", CodeContext);
    w.Write("ActivationId = {0}, ", ActivationId);
    w.Write("Timeout = {0}, ", Timeout);
    w.Write("}");
}
