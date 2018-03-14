// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ComTStoreService : public ComTXRService
    {
    public:
        ComTStoreService(TStoreService & innerService);
        virtual ~ComTStoreService() {}

    private:
        ComPointer<IFabricStateProvider2Factory> CreateStateProvider2Factory(
            __in std::wstring const & nodeId,
            __in std::wstring const & serviceName,
            __in KAllocator & allocator) override;
    };
}
