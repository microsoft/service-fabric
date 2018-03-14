// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ServiceFactoryRegistration::ServiceFactoryRegistration(
    wstring const & originalServiceTypeName,
    ServiceTypeIdentifier const & serviceTypeId,
    ComProxyStatelessServiceFactorySPtr const & statelessServiceFactory)
    : originalServiceTypeName_(originalServiceTypeName),
    serviceTypeId_(serviceTypeId),
    statelessFactory_(statelessServiceFactory)
{
}

ServiceFactoryRegistration::ServiceFactoryRegistration(
    wstring const & originalServiceTypeName,
    ServiceTypeIdentifier const & serviceTypeId,
    ComProxyStatefulServiceFactorySPtr const & statefulServiceFactory)
    : originalServiceTypeName_(originalServiceTypeName),
    serviceTypeId_(serviceTypeId),
    statefulFactory_(statefulServiceFactory)
{
}

void ServiceFactoryRegistration::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ServiceFactoryRegistration { ");
    w.Write("OriginalServiceTypeName = {0}, ", OriginalServiceTypeName);
    w.Write("ServiceTypeId = {0}, ", ServiceTypeId);
    w.Write("StatelessFactory = {0}, ", static_cast<void*>(statelessFactory_.get()));
    w.Write("StatefulFactory = {0}", static_cast<void*>(statefulFactory_.get()));
    w.Write("}");
}
