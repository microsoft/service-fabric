// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ConfigureResourceACLsRequest::ConfigureResourceACLsRequest() 
    : sids_(),
    certificateAccessDescriptions_(),
    applicationFolders_(),
    accessMask_(0),
    nodeId_(),
    applicationCounter_(),
    timeoutTicks_(0)
{
}

ConfigureResourceACLsRequest::ConfigureResourceACLsRequest(
    vector<CertificateAccessDescription> const & certificateAccessDescriptions,
    vector<wstring> const & sids,
    DWORD accessMask,
    vector<wstring> const & applicationFolders,
    ULONG applicationCounter,
    wstring const & nodeId, 
    int64 timeoutTicks)
    : sids_(sids),
    accessMask_(accessMask),
    certificateAccessDescriptions_(certificateAccessDescriptions),
    applicationFolders_(applicationFolders),
    applicationCounter_(applicationCounter),
    nodeId_(nodeId),
    timeoutTicks_(timeoutTicks)
{
}

void ConfigureResourceACLsRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureSecurityPrincipalRequest { ");
    w.Write("SIDs = {");
    for(auto iter = sids_.begin(); iter != sids_.end(); iter++)
    {
        w.Write("{0},", *iter);
    }
    w.Write("}");
    w.Write("CertificateAccessDescriptions { ");
    for(auto iter = certificateAccessDescriptions_.begin(); iter != certificateAccessDescriptions_.end(); ++iter)
    {
        w.Write("CertificateAccessDescription = {0}", certificateAccessDescriptions_);
    }
    w.Write("}");
    w.Write("ApplicationFolders = {");
    for (auto iter = applicationFolders_.begin(); iter != applicationFolders_.end(); iter++)
    {
        w.Write("{0},", *iter);
    }
    w.Write("}");
    w.Write("AccessMask = {0}", accessMask_);
    w.Write("TimeoutTicks = {0}", timeoutTicks_);
    w.Write("}");
}
