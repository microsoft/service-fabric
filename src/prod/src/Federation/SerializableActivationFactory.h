// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class SerializableActivationFactory
    {
    public:
        static void Register(SerializableWithActivationTypes::Enum typeId, SerializableActivationFactory* pFactory);
        static SerializableWithActivation* Create(unsigned int typeId);

        virtual ~SerializableActivationFactory();

    protected:
        SerializableActivationFactory();

        virtual SerializableWithActivation* CreateNew() = 0;

    private:
        static std::map<unsigned int, SerializableActivationFactory*> s_factories;
        static Common::RwLock s_lock;
    };
}
