// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {8ABFDF71-B68E-4B07-B7E9-DD9A2C7733E3}
    static const GUID CLSID_ComGetComposeDeploymentStatusListResult = 
    {0x8abfdf71, 0xb68e, 0x4b07, { 0xb7, 0xe9, 0xdd, 0x9a, 0x2c, 0x77, 0x33, 0xe3}};
    
    class ComGetComposeDeploymentStatusListResult :
        public IFabricGetComposeDeploymentStatusListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetComposeDeploymentStatusListResult)

        COM_INTERFACE_LIST1(
            ComGetComposeDeploymentStatusListResult,
            IID_IFabricGetComposeDeploymentStatusListResult,
            IFabricGetComposeDeploymentStatusListResult)

    public:
        ComGetComposeDeploymentStatusListResult(
            std::vector<ServiceModel::ComposeDeploymentStatusQueryResult> && queryResultList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        //
        // IFabricGetComposeDeploymentStatusListResult methods
        //
        const FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_ComposeDeploymentStatusQueryList(void);

        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus(void);

    private:
        Common::ReferencePointer<FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_RESULT_LIST> dockerComposeDeploymentStatusList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetComposeDeploymentStatusListResult> ComGetComposeDeploymentStatusListResultCPtr;
}
