// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("DeployedNetworkCodePackageQueryDescription");

DeployedNetworkCodePackageQueryDescription::DeployedNetworkCodePackageQueryDescription()
    : nodeName_()
    , networkName_()
    , applicationNameFilter_(NamingUri::RootNamingUri)
    , serviceManifestNameFilter_()
    , codePackageNameFilter_()
    , queryPagingDescription_()
{
}

ErrorCode DeployedNetworkCodePackageQueryDescription::FromPublicApi(FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_DESCRIPTION const &deployedNetworkCodePackageQueryDescription)
{
    TRY_PARSE_PUBLIC_STRING(deployedNetworkCodePackageQueryDescription.NodeName, nodeName_);
    TRY_PARSE_PUBLIC_STRING(deployedNetworkCodePackageQueryDescription.NetworkName, networkName_);
    
    ErrorCode error = ErrorCodeValue::Success;
    if (deployedNetworkCodePackageQueryDescription.ApplicationNameFilter != nullptr)
    {
        error = NamingUri::TryParse(deployedNetworkCodePackageQueryDescription.ApplicationNameFilter, "ApplicationNameFilter", applicationNameFilter_);
    }

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "DeployedNetworkCodePackageQueryDescription::FromPublicApi failed to parse ApplicationNameFilter = {0} error = {1}. errormessage = {2}", deployedNetworkCodePackageQueryDescription.ApplicationNameFilter, error, error.Message);
        return error;
    }

    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(deployedNetworkCodePackageQueryDescription.ServiceManifestNameFilter, serviceManifestNameFilter_);
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(deployedNetworkCodePackageQueryDescription.CodePackageNameFilter, codePackageNameFilter_);
    
    if (deployedNetworkCodePackageQueryDescription.PagingDescription != nullptr)
    {
        QueryPagingDescription pagingDescription;
        
        error = pagingDescription.FromPublicApi(*(deployedNetworkCodePackageQueryDescription.PagingDescription));
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceComponent, "PagingDescription::FromPublicApi error: {0}", error);
            return error;
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCode::Success();
}

void DeployedNetworkCodePackageQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    // NodeName is a mandatory query parameter.
    argMap.Insert(
        QueryResourceProperties::Node::Name,
        nodeName_);

    // NetworkName is a mandatory query parameter.
    argMap.Insert(
        QueryResourceProperties::Network::NetworkName,
        networkName_);

    if (!applicationNameFilter_.IsRootNamingUri)
    {
        argMap.Insert(
            QueryResourceProperties::Application::ApplicationName,
            applicationNameFilter_.ToString());
    }

    if (!serviceManifestNameFilter_.empty())
    {
        argMap.Insert(
            QueryResourceProperties::ServiceType::ServiceManifestName,
            serviceManifestNameFilter_);
    }

    if (!codePackageNameFilter_.empty())
    {
        argMap.Insert(
            QueryResourceProperties::CodePackage::CodePackageName,
            codePackageNameFilter_);
    }

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}