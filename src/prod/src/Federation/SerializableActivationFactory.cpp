// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Federation;


class FactoryCollection
{
public:
    FactoryCollection()
    {
    }

    void Register(SerializableWithActivationTypes::Enum typeId, SerializableActivationFactory* pFactory)
    {
        AcquireExclusiveLock grab(lock_);
        factories_[typeId] = pFactory;
    }

    SerializableActivationFactory* Get(unsigned int typeId)
    {
        AcquireReadLock grab(lock_);
        auto it = factories_.find(typeId);
        if (it == factories_.end())
        {
            return nullptr;
        }

        return it->second;
    }

private:
    map<unsigned int, SerializableActivationFactory*> factories_;
    RwLock lock_;
};

FactoryCollection* StaticInit_Singleton()
{
    static FactoryCollection* result = new FactoryCollection();
    return result;
}

FactoryCollection* Singleton = StaticInit_Singleton();

SerializableActivationFactory::SerializableActivationFactory()
{
}

SerializableActivationFactory::~SerializableActivationFactory()
{
}

void SerializableActivationFactory::Register(SerializableWithActivationTypes::Enum typeId, SerializableActivationFactory* pFactory)
{
    StaticInit_Singleton()->Register(typeId, pFactory);
}

SerializableWithActivation* SerializableActivationFactory::Create(unsigned int typeId)
{
    SerializableActivationFactory* factory = Singleton->Get(typeId);
    if (!factory)
    {
        return nullptr;
    }

    return factory->CreateNew();
}
