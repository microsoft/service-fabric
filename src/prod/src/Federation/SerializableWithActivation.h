// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    namespace SerializableWithActivationTypes
    {
        enum Enum : unsigned int
        {
            Invalid = 0,
            LookupVersion = 1,
            Sequence = 2,
            WString = 3
        };

        void WriteToTextWriter(Common::TextWriter & w, Enum const & val);
    }

    class SerializableWithActivation : public Serialization::FabricSerializable
    {
    public:
        static Serialization::IFabricSerializable* FabricSerializerActivator(Serialization::FabricTypeInformation typeInformation);

        __declspec (property(get = getTypeId)) SerializableWithActivationTypes::Enum TypeId;
        SerializableWithActivationTypes::Enum getTypeId() const
        {
            return typeId_;
        }

        virtual NTSTATUS GetTypeInformation(__out Serialization::FabricTypeInformation & typeInformation) const;

    protected:
        SerializableWithActivation(SerializableWithActivationTypes::Enum typeId);

    private:
        SerializableWithActivationTypes::Enum typeId_;
    };

    typedef std::unique_ptr<SerializableWithActivation> SerializableWithActivationUPtr;

    class SerializableWithActivationList : public Serialization::FabricSerializable
    {
    public:
        SerializableWithActivationList();

        SerializableWithActivationList(SerializableWithActivationList && other);

        SerializableWithActivationList & operator=(SerializableWithActivationList && other);

        void Add(SerializableWithActivationUPtr && objectUPtr);

        __declspec (property(get = getObjects)) std::vector<SerializableWithActivationUPtr> const & Objects;
        std::vector<SerializableWithActivationUPtr> const & getObjects()
        {
            return objects_;
        }

        FABRIC_FIELDS_01(objects_);

    private:
        std::vector<SerializableWithActivationUPtr> objects_;
    };
}
