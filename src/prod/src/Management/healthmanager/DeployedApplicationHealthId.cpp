// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

DeployedApplicationHealthId::DeployedApplicationHealthId()
    : applicationName_()
    , nodeId_()
{
}

DeployedApplicationHealthId::DeployedApplicationHealthId(
    std::wstring const & applicationName,
    NodeHealthId const & nodeId)
    : applicationName_(applicationName)
    , nodeId_(nodeId)
{
}

DeployedApplicationHealthId::DeployedApplicationHealthId(
    std::wstring && applicationName,
    NodeHealthId const & nodeId)
    : applicationName_(move(applicationName))
    , nodeId_(nodeId)
{
}

DeployedApplicationHealthId::DeployedApplicationHealthId(DeployedApplicationHealthId const & other)
    : applicationName_(other.applicationName_)
    , nodeId_(other.nodeId_)
{
}

DeployedApplicationHealthId & DeployedApplicationHealthId::operator = (DeployedApplicationHealthId const & other)
{
    if (this != &other)
    {
        applicationName_ = other.applicationName_;
        nodeId_ = other.nodeId_;
    }

    return *this;
}

DeployedApplicationHealthId::DeployedApplicationHealthId(DeployedApplicationHealthId && other)
    : applicationName_(move(other.applicationName_)) 
    , nodeId_(move(other.nodeId_))
{
}

DeployedApplicationHealthId & DeployedApplicationHealthId::operator = (DeployedApplicationHealthId && other)
{
    if (this != &other)
    {
        applicationName_ = move(other.applicationName_);
        nodeId_ = move(other.nodeId_);
     }

    return *this;
}

DeployedApplicationHealthId::~DeployedApplicationHealthId()
{
}

bool DeployedApplicationHealthId::operator == (DeployedApplicationHealthId const & other) const
{
    return applicationName_ == other.applicationName_ &&
           nodeId_ == other.nodeId_;
}

bool DeployedApplicationHealthId::operator != (DeployedApplicationHealthId const & other) const
{
    return !(*this == other);
}

bool DeployedApplicationHealthId::operator < (DeployedApplicationHealthId const & other) const
{
    if (applicationName_ == other.applicationName_)
    {
        return nodeId_ < other.nodeId_;
    }
    else
    {
        return applicationName_ < other.applicationName_;
    }
}

void DeployedApplicationHealthId::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write(
        "{0}{1}{2}",
        applicationName_,
        Constants::TokenDelimeter,
        nodeId_);
}

std::wstring DeployedApplicationHealthId::ToString() const
{
    return wformatString(*this);
}

// Test hooks
bool DeployedApplicationHealthId::TryParse(
    std::wstring const & entityIdStr,
    __inout wstring & applicationName,
    __inout NodeHealthId & nodeId)
{
    std::vector<wstring> tokens;
    StringUtility::Split<wstring>(entityIdStr, tokens, Constants::TokenDelimeter);
    if (tokens.size() != 2)
    {
        return false;
    }
    
    if (!LargeInteger::TryParse(tokens[1], nodeId))
    {
        return false;
    }

    applicationName = tokens[0];
    return true;
}
