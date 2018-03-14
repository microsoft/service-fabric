// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ComposeDeploymentStatusQueryDescription : public Serialization::FabricSerializable
    {
        DENY_COPY(ComposeDeploymentStatusQueryDescription)

    public:
        ComposeDeploymentStatusQueryDescription();

        ComposeDeploymentStatusQueryDescription(
            std::wstring && continuationToken,
            int64 maxResults);

        ComposeDeploymentStatusQueryDescription(
            std::wstring && deploymentNameFilter_,
            std::wstring && continuationToken);

        Common::ErrorCode FromPublicApi(FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION const & queryDesc);

        __declspec(property(get=get_DeploymentNameFilter)) std::wstring const & DeploymentNameFilter;
        std::wstring const & get_DeploymentNameFilter() const { return deploymentNameFilter_; }

        __declspec(property(get=get_ContinuationToken)) std::wstring ContinuationToken;
        std::wstring const & get_ContinuationToken() const { return continuationToken_; }

        __declspec(property(get=get_MaxResults)) int64 MaxResults;
        int64 get_MaxResults() const { return maxResults_; }

    private:
        std::wstring deploymentNameFilter_;
        std::wstring continuationToken_;
        int64 maxResults_;
    };
}
