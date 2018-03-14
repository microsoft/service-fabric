// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    // {A0196A92-3EE7-4277-ADD3-C0EC04D66FAC}
    static const GUID CLSID_ComGetApplicationNameResult = 
    { 0xa0196a92, 0x3ee7, 0x4277, { 0xad, 0xd3, 0xc0, 0xec, 0x4, 0xd6, 0x6f, 0xac } };

    class ComGetApplicationNameResult :
        public IFabricGetApplicationNameResult,
        private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComGetApplicationNameResult)

        COM_INTERFACE_LIST2(
            ComGetApplicationNameResult,
            IID_IFabricGetApplicationNameResult,
            IFabricGetApplicationNameResult,
            CLSID_ComGetApplicationNameResult,
            ComGetApplicationNameResult)

    public:
        explicit ComGetApplicationNameResult(ServiceModel::ApplicationNameQueryResult && applicationName);
        virtual ~ComGetApplicationNameResult();
        
        const FABRIC_APPLICATION_NAME_QUERY_RESULT * STDMETHODCALLTYPE get_ApplicationName(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_APPLICATION_NAME_QUERY_RESULT> applicationName_;
    };
}
