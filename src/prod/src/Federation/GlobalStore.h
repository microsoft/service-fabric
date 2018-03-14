// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class GlobalStore
    {
        DENY_COPY(GlobalStore);

    public:
        GlobalStore();

        void Register(GlobalStoreProviderSPtr const & provider);
        void Close();

        SerializableWithActivationList AddOutputState(NodeInstance const* target);
        void ProcessInputState(SerializableWithActivationList & input, NodeInstance const & from);

    private:
        GlobalStoreProviderSPtr GetProvider(SerializableWithActivationTypes::Enum typeId);

        Common::RwLock lock_;
        bool closed_;
        std::vector<GlobalStoreProviderSPtr> providers_;
        std::map<SerializableWithActivationTypes::Enum, GlobalStoreProviderSPtr> providerTable_;
    };
}
