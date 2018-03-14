// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

DeployServicePackageToNodeMessage::DeployServicePackageToNodeMessage()
    : serviceManifestName_(),
    applicationTypeName_(),
    applicationTypeVersion_(),
    nodeName_(),
    packageSharingPolicies_()
{
}

DeployServicePackageToNodeMessage::DeployServicePackageToNodeMessage(
    wstring const & serviceManifestName,
    wstring const & applicationTypeName,
    wstring const & applicationTypeVersion,
    wstring const & nodeName,
    vector<PackageSharingPair> const & packageSharingPolicies)
    : serviceManifestName_(serviceManifestName),
    applicationTypeName_(applicationTypeName),
    applicationTypeVersion_(applicationTypeVersion),
    nodeName_(nodeName),
    packageSharingPolicies_(packageSharingPolicies)
{
}

DeployServicePackageToNodeMessage::DeployServicePackageToNodeMessage(
    DeployServicePackageToNodeMessage && other) 
    : serviceManifestName_(move(other.serviceManifestName_)),
    applicationTypeName_(move(other.applicationTypeName_)),
    applicationTypeVersion_(move(other.applicationTypeVersion_)),
    nodeName_(move(other.nodeName_)),
    packageSharingPolicies_(move(other.packageSharingPolicies_))
{
}

DeployServicePackageToNodeMessage const & DeployServicePackageToNodeMessage::operator= (DeployServicePackageToNodeMessage && other)
{
    if(this != &other)
    {
        this->serviceManifestName_ = move(other.serviceManifestName_);
        this->applicationTypeName_ = move(other.applicationTypeName_);
        this->applicationTypeVersion_ = move(other.applicationTypeVersion_);
        this->nodeName_ = move(other.nodeName_);
        this->packageSharingPolicies_ = move(other.packageSharingPolicies_);
    }
    return *this;
}

ErrorCode DeployServicePackageToNodeMessage::GetSharingPoliciesFromRest(__out wstring & result)
{

    ErrorCode error(ErrorCodeValue::Success);

    vector<PackageSharingPolicyQueryObject> policies;

    for (auto iter = packageSharingPolicies_.begin(); iter != packageSharingPolicies_.end(); ++iter)
    {
        ServicePackageSharingType::Enum packageSharingType;
        error = ServicePackageSharingType::FromPublic(
            (*iter).PackageSharingScope,
            packageSharingType);

        if (!error.IsSuccess()) { return error; }

        PackageSharingPolicyQueryObject policy((*iter).SharedPackageName, packageSharingType);
        policies.push_back(policy);
    }

    if (!policies.empty())
    {
        PackageSharingPolicyList policyList(policies);
        error = JsonHelper::Serialize<PackageSharingPolicyList>(policyList, result);
        if (!error.IsSuccess())
        {
            return error;
        }
    }
    return error;

}

void DeployServicePackageToNodeMessage::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("DeployServicePackageToNodeMessage { ");
    w.Write("ServiceManifestName = {0}", this->serviceManifestName_);
    w.Write("ApplicationTypeName = {0}", this->applicationTypeName_);
    w.Write("NodeName = {0}", this->nodeName_);
    w.Write("PackageSharingPolicies { ");
    for (auto iter = packageSharingPolicies_.begin(); iter != packageSharingPolicies_.end(); ++iter)
    {
        w.Write("SharedPackageName = {0}", iter->SharedPackageName);
        w.Write("PackageSharingScope = {0}", (int)iter->PackageSharingScope);
    }
    w.Write("}");
    w.Write("}");
}
