//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Api
{
    // {5F850670-BD56-4B76-998D-60E172DDA4BF}
    static const GUID CLSID_ComSecretsResult =
    { 0x5f850670, 0xbd56, 0x4b76,{ 0x99, 0x8d, 0x60, 0xe1, 0x72, 0xdd, 0xa4, 0xbf } };

    class ComSecretsResult
        : public IFabricSecretsResult
        , private ComUnknownBase
    {
        DENY_COPY(ComSecretsResult)

        BEGIN_COM_INTERFACE_LIST(ComSecretsResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricSecretsResult)
            COM_INTERFACE_ITEM(IID_IFabricSecretsResult, IFabricSecretsResult)
            COM_INTERFACE_ITEM(CLSID_ComSecretsResult, ComSecretsResult)
        END_COM_INTERFACE_LIST()

    public:
        ComSecretsResult(Management::CentralSecretService::SecretsDescription const & description)
            : IFabricSecretsResult()
            , ComUnknownBase()
            , heap_()
            , secrets_()
        {
            this->secrets_ = this->heap_.AddItem<FABRIC_SECRET_LIST>();
            description.ToPublicApi(this->heap_, *this->secrets_);
        }

        virtual ~ComSecretsResult() {}

        // 
        // IFabricSecretsResult methods
        // 
        const FABRIC_SECRET_LIST * STDMETHODCALLTYPE get_Secrets()
        {
            return this->secrets_.GetRawPointer();
        }

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_SECRET_LIST> secrets_;
    };
}