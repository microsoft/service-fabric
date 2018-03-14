// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

DeactivateCodePackageRequest::DeactivateCodePackageRequest()
    : codePackageInstanceId_(),
    activationId_(),
    timeout_()
{
}

DeactivateCodePackageRequest::DeactivateCodePackageRequest(
    CodePackageInstanceIdentifier const & codePackageId, 
    CodePackageActivationId const & activationId,
    TimeSpan const timeout)
    : codePackageInstanceId_(codePackageId),
    activationId_(activationId),
    timeout_(timeout)
{
}

void DeactivateCodePackageRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("DeactivateCodePackageRequest { ");
    w.Write("CodePackageInstanceId = {0}, ", codePackageInstanceId_);
    w.Write("ActivationId = {0}, ", ActivationId);
    w.Write("Timeout = {0}, ", Timeout);
    w.Write("}");
}
