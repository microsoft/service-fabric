// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class GlobalStoreProvider
    {
    protected:
        GlobalStoreProvider()
        {
        }

        void AddType(SerializableWithActivationTypes::Enum typeId)
        {
            types_.push_back(typeId);
        }

    public:
        __declspec (property(get = getTypes)) std::vector<SerializableWithActivationTypes::Enum> const & Types;
        std::vector<SerializableWithActivationTypes::Enum> const & getTypes() const
        {
            return types_;
        }

        virtual void AddOutputState(SerializableWithActivationList & output, NodeInstance const* target) = 0;
        virtual void ProcessInputState(SerializableWithActivation const & input, NodeInstance const & from) = 0;

    private:
        std::vector<SerializableWithActivationTypes::Enum> types_;
    };

    typedef std::shared_ptr<GlobalStoreProvider> GlobalStoreProviderSPtr;
}
