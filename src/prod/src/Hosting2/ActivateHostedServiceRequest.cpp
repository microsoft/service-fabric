// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ActivateHostedServiceRequest::ActivateHostedServiceRequest()
{
}

ActivateHostedServiceRequest::ActivateHostedServiceRequest(HostedServiceParameters const & params)
    : params_(params)
{
}

void ActivateHostedServiceRequest::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("ActivateHostedServiceRequest { ");
    w.Write("Params = {0}", params_);
    w.Write("}");
}
