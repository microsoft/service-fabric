// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

CodePackageTerminationNotificationRequest::CodePackageTerminationNotificationRequest()
    : codePackageInstanceId_(),
    activationId_()
{
}

CodePackageTerminationNotificationRequest::CodePackageTerminationNotificationRequest(
    CodePackageInstanceIdentifier const & codePackageInstanceId, 
    CodePackageActivationId const & activationId)
    : codePackageInstanceId_(codePackageInstanceId),
    activationId_(activationId)
{
}

void CodePackageTerminationNotificationRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CodePackageTerminationNotificationRequest { ");
    w.Write("CodePackageInstanceId = {0}, ", CodePackageInstanceId);
    w.Write("ActivationId = {0}, ", ActivationId);
    w.Write("}");
}
