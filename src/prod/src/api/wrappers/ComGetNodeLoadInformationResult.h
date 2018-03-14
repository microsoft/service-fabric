// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {5CAE4128-B670-4F09-905B-3B6CAFA36E93}
    static const GUID CLSID_ComGetNodeLoadInformationResult =
    {0x5cae4128,0xb670,0x4f09,{0x90,0x5b,0x3b,0x6c,0xaf,0xa3,0x6e,0x93}};
    
    class ComGetNodeLoadInformationResult :
        public IFabricGetNodeLoadInformationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetNodeLoadInformationResult)

        COM_INTERFACE_LIST2(
            ComGetNodeLoadInformationResult,
            IID_IFabricGetNodeLoadInformationResult,
            IFabricGetNodeLoadInformationResult,
            CLSID_ComGetNodeLoadInformationResult,
            ComGetNodeLoadInformationResult)

    public:
        explicit ComGetNodeLoadInformationResult(ServiceModel::NodeLoadInformationQueryResult && NodeLoadInformation);

        // 
        // IFabricGetNodeLoadInformationResult methods
        // 
        const FABRIC_NODE_LOAD_INFORMATION *STDMETHODCALLTYPE get_NodeLoadInformation( void);

    private:
        Common::ReferencePointer<FABRIC_NODE_LOAD_INFORMATION> NodeLoadInformation_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetNodeLoadInformationResult> ComGetNodeLoadInformationResultCPtr;
}
