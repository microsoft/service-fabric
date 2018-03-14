// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

DeactivateHostedServiceRequest::DeactivateHostedServiceRequest()
    : serviceName_(),
    graceful_()
{
}

DeactivateHostedServiceRequest::DeactivateHostedServiceRequest(wstring const & serviceName, bool graceful)
    : serviceName_(serviceName),
    graceful_(graceful)
{
}

void DeactivateHostedServiceRequest::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("DeactivateHostedServiceRequest { ");
    w.Write("ServiceName = {0}", serviceName_);
    w.Write("Graceful = {0}", graceful_);
    w.Write("}");
}
