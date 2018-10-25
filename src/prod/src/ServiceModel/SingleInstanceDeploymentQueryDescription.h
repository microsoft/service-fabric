//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class SingleInstanceDeploymentQueryDescription : public Serialization::FabricSerializable
    {
        DENY_COPY(SingleInstanceDeploymentQueryDescription)

    public:
        SingleInstanceDeploymentQueryDescription();

        SingleInstanceDeploymentQueryDescription(std::wstring const & deploymentNameFilter);

        SingleInstanceDeploymentQueryDescription(DWORD const deploymentTypeFilter);

        SingleInstanceDeploymentQueryDescription(
            DWORD const deploymentTypeFilter,
            std::wstring && continuationToken,
            int64 maxResults);

        __declspec(property(get=get_DeploymentNameFilter)) std::wstring const & DeploymentNameFilter;
        std::wstring const & get_DeploymentNameFilter() const { return deploymentNameFilter_; }

        __declspec(property(get=get_DeploymentTypeFilter)) DWORD DeploymentTypeFilter;
        DWORD get_DeploymentTypeFilter() const { return deploymentTypeFilter_; }

        __declspec(property(get=get_ContinuationToken)) std::wstring const & ContinuationToken;
        std::wstring const & get_ContinuationToken() const { return continuationToken_; }

        __declspec(property(get=get_MaxResults)) int64 MaxResults;
        int64 get_MaxResults() const { return maxResults_; }

    private:
        std::wstring deploymentNameFilter_;
        DWORD deploymentTypeFilter_;
        std::wstring continuationToken_;
        int64 maxResults_;
    };
}
