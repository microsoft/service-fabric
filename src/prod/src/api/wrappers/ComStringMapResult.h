// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComStringMapResult
        : public IFabricStringMapResult
        , protected Common::ComUnknownBase
    {
        DENY_COPY(ComStringMapResult)

        COM_INTERFACE_LIST1(
            ComStringMapResult,
            IID_IFabricStringMapResult,
            IFabricStringMapResult)

    public:

        ComStringMapResult(std::map<std::wstring, std::wstring> &&);
        virtual ~ComStringMapResult();

        //
        // IFabricStringMapResult
        //
        FABRIC_STRING_PAIR_LIST * STDMETHODCALLTYPE get_Result(void);

    private:
        std::map<std::wstring, std::wstring> map_;
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_STRING_PAIR_LIST> result_;
        Common::RwLock lock_;
    };
}
