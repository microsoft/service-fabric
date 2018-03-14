// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{

    // f92759fe-1917-4b51-923f-7e8e7b04d838
    static const GUID CLSID_ComServiceGroupDescriptionResult =
    {0xf92759fe,0x1917,0x4b51,{0x92, 0x3f, 0x7e, 0x8e, 0x7b, 0x04, 0xd8, 0x38}};
    
    class ComServiceGroupDescriptionResult
        : public IFabricServiceGroupDescriptionResult,
          private Common::ComUnknownBase
    {
        DENY_COPY_ASSIGNMENT(ComServiceGroupDescriptionResult)

        COM_INTERFACE_LIST2(
            ComServiceGroupDescriptionResult,
            IID_IFabricServiceGroupDescriptionResult,
            IFabricServiceGroupDescriptionResult,
            CLSID_ComServiceGroupDescriptionResult,
            ComServiceGroupDescriptionResult)

    public:

        explicit ComServiceGroupDescriptionResult(Naming::ServiceGroupDescriptor const & nativeResult);

        const FABRIC_SERVICE_GROUP_DESCRIPTION * STDMETHODCALLTYPE get_Description(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_SERVICE_GROUP_DESCRIPTION> description_;
    };
}
