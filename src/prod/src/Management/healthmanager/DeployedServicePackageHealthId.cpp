// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;
using namespace ServiceModel;

DeployedServicePackageHealthId::DeployedServicePackageHealthId()
    : applicationName_()
    , serviceManifestName_()
    , servicePackageActivationId_()
{
}

DeployedServicePackageHealthId::DeployedServicePackageHealthId(
    std::wstring const & applicationName,
    std::wstring const & serviceManifestName,
    std::wstring const & servicePackageActivationId,
    NodeHealthId const & nodeId)
    : applicationName_(applicationName)
    , serviceManifestName_(serviceManifestName)
    , servicePackageActivationId_(servicePackageActivationId)
    , nodeId_(nodeId)
{
}

DeployedServicePackageHealthId::DeployedServicePackageHealthId(
    std::wstring && applicationName,
    std::wstring && serviceManifestName,
    std::wstring && servicePackageActivationId,
    NodeHealthId const & nodeId)
    : applicationName_(move(applicationName))
    , serviceManifestName_(move(serviceManifestName))
    , servicePackageActivationId_(move(servicePackageActivationId))
    , nodeId_(nodeId)
{
}

DeployedServicePackageHealthId::DeployedServicePackageHealthId(DeployedServicePackageHealthId const & other)
    : applicationName_(other.applicationName_)
    , serviceManifestName_(other.serviceManifestName_)
    , servicePackageActivationId_(other.servicePackageActivationId_)
    , nodeId_(other.nodeId_)
{
}

DeployedServicePackageHealthId & DeployedServicePackageHealthId::operator = (DeployedServicePackageHealthId const & other)
{
    if (this != &other)
    {
        applicationName_ = other.applicationName_;
        serviceManifestName_ = other.serviceManifestName_;
        servicePackageActivationId_ = other.servicePackageActivationId_;
        nodeId_ = other.nodeId_;
    }

    return *this;
}

DeployedServicePackageHealthId::DeployedServicePackageHealthId(DeployedServicePackageHealthId && other)
    : applicationName_(move(other.applicationName_)) 
    , serviceManifestName_(move(other.serviceManifestName_))
    , servicePackageActivationId_(move(other.servicePackageActivationId_))
    , nodeId_(move(other.nodeId_))
{
}

DeployedServicePackageHealthId & DeployedServicePackageHealthId::operator = (DeployedServicePackageHealthId && other)
{
    if (this != &other)
    {
        applicationName_ = move(other.applicationName_);
        serviceManifestName_ = move(other.serviceManifestName_);
        servicePackageActivationId_ = move(other.servicePackageActivationId_);
        nodeId_ = move(other.nodeId_);
     }

    return *this;
}

DeployedServicePackageHealthId::~DeployedServicePackageHealthId()
{
}

bool DeployedServicePackageHealthId::operator == (DeployedServicePackageHealthId const & other) const
{
    return applicationName_ == other.applicationName_ &&
           nodeId_ == other.nodeId_ &&
           serviceManifestName_ == other.serviceManifestName_ &&
           servicePackageActivationId_ == other.servicePackageActivationId_;
}

bool DeployedServicePackageHealthId::operator != (DeployedServicePackageHealthId const & other) const
{
    return !(*this == other);
}

bool DeployedServicePackageHealthId::operator < (DeployedServicePackageHealthId const & other) const
{
    if (nodeId_ == other.nodeId_)
    {
        if (applicationName_ == other.applicationName_)
        {
            if (serviceManifestName_ == other.serviceManifestName_)
            {
                return servicePackageActivationId_ < other.servicePackageActivationId_;
            }

            return serviceManifestName_ < other.serviceManifestName_;             
        }
        
        return applicationName_ < other.applicationName_;
    }
    
    return nodeId_ < other.nodeId_;
}

void DeployedServicePackageHealthId::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    if (servicePackageActivationId_.empty())
    {
        w.Write(
            "{0}{1}{2}{3}{4}",
            applicationName_,
            Constants::TokenDelimeter,
            serviceManifestName_,
            Constants::TokenDelimeter,
            nodeId_);
    }
    else
    {
        w.Write(
            "{0}{1}{2}{3}{4}{5}{6}",
            applicationName_,
            Constants::TokenDelimeter,
            serviceManifestName_,
            Constants::TokenDelimeter,
            servicePackageActivationId_,
            Constants::TokenDelimeter,
            nodeId_);
    }
}

std::wstring DeployedServicePackageHealthId::ToString() const
{
    return wformatString(*this);
}

// Test hooks
bool DeployedServicePackageHealthId::TryParse(
    std::wstring const & entityIdStr,
    __inout wstring & applicationName,
    __inout wstring & serviceManifestName,
    __inout wstring & servicePackageActivationId,
    __inout NodeHealthId & nodeId)
{
    std::vector<wstring> tokens;
    StringUtility::Split<wstring>(entityIdStr, tokens, Constants::TokenDelimeter, false); // servicePackageActivationId can be empty.
    if (tokens.size() != 4)
    {
        return false;
    }
    
    if (!LargeInteger::TryParse(tokens[3], nodeId))
    {
        return false;
    }

    applicationName = tokens[0];
    serviceManifestName = tokens[1];
    servicePackageActivationId = tokens[2];

    return true;
}
