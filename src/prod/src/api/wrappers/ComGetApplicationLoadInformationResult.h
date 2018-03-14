// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {0B60B46E-4AE4-4073-8C06-BBD49966A549}
    static const GUID CLSID_ComGetApplicationLoadInformationResult =
    { 0xb60b46e, 0x4ae4, 0x4073, { 0x8c, 0x6, 0xbb, 0xd4, 0x99, 0x66, 0xa5, 0x49 } };

    class ComGetApplicationLoadInformationResult :
        public IFabricGetApplicationLoadInformationResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetApplicationLoadInformationResult)

        COM_INTERFACE_LIST2(
            ComGetApplicationLoadInformationResult,
            IID_IFabricGetApplicationLoadInformationResult,
            IFabricGetApplicationLoadInformationResult,
            CLSID_ComGetApplicationLoadInformationResult,
            ComGetApplicationLoadInformationResult)

    public:
        explicit ComGetApplicationLoadInformationResult(ServiceModel::ApplicationLoadInformationQueryResult && applicationLoadInformation);

        // 
        // IFabricGetApplicationLoadInformationResult methods
        // 
        const FABRIC_APPLICATION_LOAD_INFORMATION *STDMETHODCALLTYPE get_ApplicationLoadInformation( void);

    private:
        Common::ReferencePointer<FABRIC_APPLICATION_LOAD_INFORMATION> applicationLoadInformation_;
        Common::ScopedHeap heap_;
    };

    typedef Common::ComPointer<ComGetApplicationLoadInformationResult> ComGetApplicationLoadInformationResultCPtr;
}
