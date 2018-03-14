// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Serialization;
using namespace Common;
using namespace Federation;

namespace Federation
{
    namespace SerializableWithActivationTypes
    {
        void WriteToTextWriter(TextWriter & w, Enum const & val)
        {
            switch (val)
            {
            case LookupVersion:
                w << "LookupVersion";
                return;
            case Sequence:
                w << "Sequence";
                return;
            case WString:
                w << "WString";
                return;
            default:
                w << "Invalid(" << static_cast<int>(val) << ')';
                return;
            }
        }
    }
}

SerializableWithActivation::SerializableWithActivation(SerializableWithActivationTypes::Enum typeId)
    : typeId_(typeId)
{
}

NTSTATUS SerializableWithActivation::GetTypeInformation(__out FabricTypeInformation & typeInformation) const
{
    typeInformation.buffer = reinterpret_cast<UCHAR const *>(&typeId_);

    int i;
    for (i = 3; i > 0 && typeInformation.buffer[i] == 0; i--);

    typeInformation.length = i + 1;

    return STATUS_SUCCESS;
}

IFabricSerializable* SerializableWithActivation::FabricSerializerActivator(FabricTypeInformation typeInformation)
{
    unsigned int typeId = 0;
    for (int i = static_cast<int>(typeInformation.length) - 1; i >= 0; i--)
    {
        typeId <<= 8;
        typeId += typeInformation.buffer[i];
    }

    return SerializableActivationFactory::Create(typeId);
}

SerializableWithActivationList::SerializableWithActivationList()
{
}

SerializableWithActivationList::SerializableWithActivationList(SerializableWithActivationList && other)
    : objects_(move(other.objects_))
{
}

SerializableWithActivationList & SerializableWithActivationList::operator = (SerializableWithActivationList && other)
{
    if (this != &other)
    {
        this->objects_ = std::move(other.objects_);
    }

    return *this;
}

void SerializableWithActivationList::Add(SerializableWithActivationUPtr && objectUPtr)
{
    objects_.push_back(std::move(objectUPtr));
}
