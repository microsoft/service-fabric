//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Api
{
    // {99387883-EC9D-4DC1-95CE-605BA01903E0}
    static const GUID CLSID_ComSecretReferencesResult =
    { 0x99387883, 0xec9d, 0x4dc1,{ 0x95, 0xce, 0x60, 0x5b, 0xa0, 0x19, 0x3, 0xe0 } };

    class ComSecretReferencesResult
        : public IFabricSecretReferencesResult
        , private Common::ComUnknownBase
    {
        DENY_COPY(ComSecretReferencesResult)

        BEGIN_COM_INTERFACE_LIST(ComSecretReferencesResult)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricSecretReferencesResult)
            COM_INTERFACE_ITEM(IID_IFabricSecretReferencesResult, IFabricSecretReferencesResult)
            COM_INTERFACE_ITEM(CLSID_ComSecretReferencesResult, ComSecretReferencesResult)
        END_COM_INTERFACE_LIST()

    public:
        ComSecretReferencesResult(Management::CentralSecretService::SecretReferencesDescription const & description)
            : IFabricSecretReferencesResult()
            , ComUnknownBase()
            , heap_()
            , secretReferences_()
        {
            this->secretReferences_ = this->heap_.AddItem<FABRIC_SECRET_REFERENCE_LIST>();
            description.ToPublicApi(this->heap_, *this->secretReferences_);
        }

        virtual ~ComSecretReferencesResult() {}

        // 
        // IFabricSecretReferencesResult methods
        // 
        const FABRIC_SECRET_REFERENCE_LIST * STDMETHODCALLTYPE get_SecretReferences()
        {
            return this->secretReferences_.GetRawPointer();
        }

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_SECRET_REFERENCE_LIST> secretReferences_;
    };
}