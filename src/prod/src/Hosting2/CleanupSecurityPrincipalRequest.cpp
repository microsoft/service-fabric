// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

CleanupSecurityPrincipalRequest::CleanupSecurityPrincipalRequest() 
    : nodeId_(),
    applicationIds_(),
    cleanupForNode_(false)
{
}

CleanupSecurityPrincipalRequest::CleanupSecurityPrincipalRequest(
    wstring const & nodeId,
    vector<wstring> const & applicationIds,
    bool const cleanupForNode)
    : nodeId_(nodeId),
    applicationIds_(applicationIds),
    cleanupForNode_(cleanupForNode)
{
}

void CleanupSecurityPrincipalRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("CleanupSecurityPrincipalRequest { ");
    w.Write("NodeId = {0}", nodeId_);
    w.Write("CleanupForNode = {0}", cleanupForNode_);
    for (auto iter = applicationIds_.begin(); iter != applicationIds_.end(); ++iter)
    {
        w.Write("ApplicationId = {0}", (*iter));
    }
    w.Write("}");
}
