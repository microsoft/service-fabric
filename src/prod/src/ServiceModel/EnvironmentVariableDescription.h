// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
	struct EnvironmentVariablesDescription;
    struct EnvironmentOverridesDescription;
    // <EnvironmentVariable> element.
    struct EnvironmentVariableDescription : 
		public Serialization::FabricSerializable,
        public Common::IFabricJsonSerializable
    {
    public:
        EnvironmentVariableDescription(); 

        EnvironmentVariableDescription(EnvironmentVariableDescription const & other);
        EnvironmentVariableDescription(EnvironmentVariableDescription && other);

        EnvironmentVariableDescription & operator = (EnvironmentVariableDescription const & other) = default;
        EnvironmentVariableDescription & operator = (EnvironmentVariableDescription && other) = default;

        bool operator == (EnvironmentVariableDescription const & other) const;
        bool operator != (EnvironmentVariableDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::nameCamelCase, Name)
            SERIALIZABLE_PROPERTY(Constants::valueCamelCase, Value)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(Name)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(Value)
        END_DYNAMIC_SIZE_ESTIMATION()

        void clear();

        FABRIC_FIELDS_03(Name, Value, Type);

    public:
        std::wstring Name;
        std::wstring Value;
        std::wstring Type;

    private:
        friend struct ServiceModel::EnvironmentOverridesDescription;
		friend struct ServiceModel::EnvironmentVariablesDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::EnvironmentVariableDescription);
