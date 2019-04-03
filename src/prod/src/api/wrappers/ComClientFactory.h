// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Api
{
    class ComClientFactory
    {
    public:
        ComClientFactory(IClientFactoryPtr const& impl);
        virtual ~ComClientFactory();

        HRESULT CreateComFabricClient(
            __RPC__in REFIID iid,
            __RPC__deref_out_opt void **fabricClient);

    private:
        IClientFactoryPtr impl_;
    };
}
