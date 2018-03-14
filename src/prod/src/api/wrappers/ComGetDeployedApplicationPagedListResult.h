// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // All the list result wrappers in this project have this same GUID
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E92}
    static const GUID CLSID_ComGetDeployedApplicationPagedListResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x92}};

    class ComGetDeployedApplicationPagedListResult :
        public IFabricGetDeployedApplicationPagedListResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetDeployedApplicationPagedListResult)

        COM_INTERFACE_LIST2(
            ComGetDeployedApplicationPagedListResult,
            IID_IFabricGetDeployedApplicationPagedListResult,
            IFabricGetDeployedApplicationPagedListResult,
            CLSID_ComGetDeployedApplicationPagedListResult,
            ComGetDeployedApplicationPagedListResult)

    public:
        explicit ComGetDeployedApplicationPagedListResult(
            std::vector<ServiceModel::DeployedApplicationQueryResult> && deployedApplicationList,
            ServiceModel::PagingStatusUPtr && pagingStatus);

        //
        // IFabricGetDeployedApplicationPagedListResult methods
        //
        const FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST *STDMETHODCALLTYPE get_DeployedApplicationPagedList(void);
        const FABRIC_PAGING_STATUS *STDMETHODCALLTYPE get_PagingStatus(void);

    private:
        Common::ReferencePointer<FABRIC_DEPLOYED_APPLICATION_QUERY_RESULT_LIST> deployedApplicationList_;
        Common::ReferencePointer<FABRIC_PAGING_STATUS> pagingStatus_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetDeployedApplicationPagedListResult> ComGetDeployedApplicationPagedListResultCPtr;
}
