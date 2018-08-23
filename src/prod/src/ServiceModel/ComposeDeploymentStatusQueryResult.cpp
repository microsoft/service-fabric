// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

INITIALIZE_SIZE_ESTIMATION(ComposeDeploymentStatusQueryResult)

ComposeDeploymentStatusQueryResult::ComposeDeploymentStatusQueryResult(
    wstring const & deploymentName,
    NamingUri const & applicationName,
    ComposeDeploymentStatus::Enum dockerComposeDeploymentStatus,
    wstring const & statusDetails)
    : deploymentName_(deploymentName)
    , applicationName_(applicationName)
    , dockerComposeDeploymentStatus_(dockerComposeDeploymentStatus)
    , statusDetails_(statusDetails)
{
}

void ComposeDeploymentStatusQueryResult::ToPublicApi(
    __in ScopedHeap & heap,
    __out FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM & publicQueryResult) const
{
    publicQueryResult.DeploymentName = heap.AddString(deploymentName_);

    publicQueryResult.ApplicationName = heap.AddString(applicationName_.ToString());

    publicQueryResult.Status = ComposeDeploymentStatus::ToPublicApi(dockerComposeDeploymentStatus_);
    
    publicQueryResult.StatusDetails = heap.AddString(statusDetails_);
}

ErrorCode ComposeDeploymentStatusQueryResult::FromPublicApi(__in FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_ITEM const & publicComposeQueryResult)
{
    auto hr = StringUtility::LpcwstrToWstring(publicComposeQueryResult.DeploymentName, false, deploymentName_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    if (!NamingUri::TryParse(publicComposeQueryResult.ApplicationName, applicationName_))
    {
        return ErrorCodeValue::InvalidArgument;
    }

    dockerComposeDeploymentStatus_ = ComposeDeploymentStatus::FromPublicApi(publicComposeQueryResult.Status);

    hr = StringUtility::LpcwstrToWstring(publicComposeQueryResult.StatusDetails, true, statusDetails_);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    return ErrorCode::Success();
}

void ComposeDeploymentStatusQueryResult::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring ComposeDeploymentStatusQueryResult::ToString() const
{
    return wformatString("DeploymentName = '{0}', ApplicationName = '{1}', ComposeDeploymentStatus = '{2}' StatusDetails = '{3}'",
        deploymentName_,
        applicationName_,
        dockerComposeDeploymentStatus_,
        statusDetails_);
}

wstring ComposeDeploymentStatusQueryResult::CreateContinuationToken() const
{
    return deploymentName_;
}
