// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ConfigureNodeForDnsServiceRequest::ConfigureNodeForDnsServiceRequest()
    : isDnsServiceEnabled_(false),
    setAsPreferredDns_(false)
{
}

ConfigureNodeForDnsServiceRequest::ConfigureNodeForDnsServiceRequest(
    bool isDnsServiceEnabled,
    bool setAsPreferredDns,
    std::wstring const & sid)
    : isDnsServiceEnabled_(isDnsServiceEnabled),
    setAsPreferredDns_(setAsPreferredDns),
    sid_(sid)
{
}

void ConfigureNodeForDnsServiceRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureNodeForDnsServiceRequest { ");
    w.Write("IsDnsServiceEnabled = {0} setAsPreferredDns = {1} Sid = {2}",
        isDnsServiceEnabled_, setAsPreferredDns_, sid_);
    w.Write("}");
}
