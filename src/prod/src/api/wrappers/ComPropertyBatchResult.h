// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    //dedafcd9-914d-47c5-8120-fbd8b34c4fae
    static const GUID CLSID_ComPropertyBatchResult =
    {0xdedafcd9, 0x914d, 0x47c5,  {0x81, 0x20, 0xfb, 0xd8, 0xb3, 0x4c, 0x4f, 0xae}};

    class ComPropertyBatchResult
        : public IFabricPropertyBatchResult,
          private Common::ComUnknownBase
    {
        DENY_COPY(ComPropertyBatchResult);

        BEGIN_COM_INTERFACE_LIST(ComPropertyBatchResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricPropertyBatchResult)
            COM_INTERFACE_ITEM(IID_IFabricPropertyBatchResult, IFabricPropertyBatchResult)
            COM_INTERFACE_ITEM(CLSID_ComPropertyBatchResult, ComPropertyBatchResult)
        END_COM_INTERFACE_LIST()

    public:
        ComPropertyBatchResult(
            IPropertyBatchResultPtr const &impl);

        HRESULT STDMETHODCALLTYPE GetProperty(
            ULONG operationIndexInRequest,
            __out IFabricPropertyValueResult ** property);

    private:
         IPropertyBatchResultPtr impl_;
    };
}
