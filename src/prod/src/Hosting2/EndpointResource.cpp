// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace Management;
using namespace ServiceModel;

EndpointResource::EndpointResource(
    EndpointDescription const & endpointDescription,
    EndpointBindingPolicyDescription const & endpointBindingPolicy) 
    : endpointDescription_(endpointDescription),
    endpointBindingPolicy_(endpointBindingPolicy)
{
}

EndpointResource::~EndpointResource()
{
}

void EndpointResource::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("Endpoint { ");
    w.Write("Description = {0}, ", this->endpointDescription_);
    w.Write("EndpointBindingPolicy = {0}", this->endpointBindingPolicy_);
    w.Write("AccessEntries = ");
    for(auto it = AccessEntries.cbegin(); it != AccessEntries.cend(); ++it)
    {
        w.Write("[{0}:{1}],", it->PrincipalInfo->Name, it->AccessType);
    }
    w.Write("}");
}
