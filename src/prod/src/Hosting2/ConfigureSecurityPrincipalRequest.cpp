// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;
using namespace std;

ConfigureSecurityPrincipalRequest::ConfigureSecurityPrincipalRequest()
    : nodeId_(),
    applicationId_(),
    applicationPackageCounter_(),
    principalsDescription_(),
    allowedUserCreationFailureCount_(0),
    configureForNode_(false),
    updateExisting_(false)
{
}

ConfigureSecurityPrincipalRequest::ConfigureSecurityPrincipalRequest(
    wstring const & nodeId,
    wstring const & applicationId,
    ULONG applicationPackageCounter,
    PrincipalsDescription const & principalsDescription,
    int const allowedUserCreationFailureCount,
    bool const configureForNode,
    bool const updateExisting)
    : nodeId_(nodeId),
    applicationId_(applicationId),
    applicationPackageCounter_(applicationPackageCounter),
    principalsDescription_(principalsDescription),
    allowedUserCreationFailureCount_(allowedUserCreationFailureCount),
    configureForNode_(configureForNode),
    updateExisting_(updateExisting)
{
}

ConfigureSecurityPrincipalRequest::ConfigureSecurityPrincipalRequest(ConfigureSecurityPrincipalRequest && other)
    : nodeId_(move(other.nodeId_)),
    applicationId_(move(other.applicationId_)),
    applicationPackageCounter_(move(other.applicationPackageCounter_)),
    principalsDescription_(move(other.principalsDescription_)),
    allowedUserCreationFailureCount_(move(other.allowedUserCreationFailureCount_)),
    configureForNode_(move(other.configureForNode_)),
    updateExisting_(move(other.updateExisting_))
{
}


ConfigureSecurityPrincipalRequest & ConfigureSecurityPrincipalRequest::operator=(ConfigureSecurityPrincipalRequest && other)
{
    if (this != &other)
    {
        nodeId_ = move(other.nodeId_);
        applicationId_ = move(other.applicationId_);
        applicationPackageCounter_ = move(other.applicationPackageCounter_);
        principalsDescription_ = move(other.principalsDescription_);
        allowedUserCreationFailureCount_ = move(other.allowedUserCreationFailureCount_);
        configureForNode_ = move(other.configureForNode_);
        updateExisting_ = move(other.updateExisting_);
    }

    return *this;
}

bool ConfigureSecurityPrincipalRequest::HasEmptyGroupsAndUsers() const
{
    return principalsDescription_.Groups.empty() && principalsDescription_.Users.empty();
}

void ConfigureSecurityPrincipalRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureSecurityPrincipalRequest { ");
    w.Write("NodeId = {0}", nodeId_);
    w.Write("ApplicationId = {0}", applicationId_);
    w.Write("ApplicationPackageCounter = {0}", applicationPackageCounter_);
    w.Write("PrincipalDescription = {0}", principalsDescription_);
    w.Write("AllowedUserCreationFailureCount = {0}", allowedUserCreationFailureCount_);
    w.Write("ConfigureForNode = {0}", configureForNode_);
    w.Write("UpdateExisting = {0}", updateExisting_);
    w.Write("}");
}
