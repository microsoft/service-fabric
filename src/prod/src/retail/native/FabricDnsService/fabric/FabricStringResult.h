// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    using ::_delete;

    class FabricStringResult :
        public KShared<FabricStringResult>,
        public IFabricStringResult
    {
        K_FORCE_SHARED(FabricStringResult);

        K_BEGIN_COM_INTERFACE_LIST(FabricStringResult)
            K_COM_INTERFACE_ITEM(__uuidof(IUnknown), IFabricStringResult)
            K_COM_INTERFACE_ITEM(IID_IFabricStringResult, IFabricStringResult)
        K_END_COM_INTERFACE_LIST()

    public:
        static void Create(
            __out FabricStringResult::SPtr& spStringResult,
            __in KAllocator& allocator,
            __in KStringView strAddress
            );

    private:
        FabricStringResult(
            __in KStringView strAddress
            );

    public:
        virtual LPCWSTR get_String() override;

    private:
        KStringView _strAddress;
    };
}
