// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    struct ScalingMechanism;
    using ScalingMechanismSPtr = std::shared_ptr<ScalingMechanism>;

    struct ScalingMechanism
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ScalingMechanism();

        ScalingMechanism(ScalingMechanismKind::Enum kind);

        ScalingMechanism(ScalingMechanism const & other) = default;
        ScalingMechanism & operator=(ScalingMechanism const & other) = default;
        ScalingMechanism(ScalingMechanism && other) = default;
        ScalingMechanism & operator=(ScalingMechanism && other) = default;

        virtual ~ScalingMechanism() = default;

        virtual bool Equals(ScalingMechanismSPtr const & other, bool ignoreDynamicContent) const;

        __declspec (property(get = get_Kind)) ScalingMechanismKind::Enum Kind;
        ScalingMechanismKind::Enum get_Kind() const { return kind_; }

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        virtual Common::ErrorCode FromPublicApi(FABRIC_SCALING_MECHANISM const & policy);

        virtual void ToPublicApi(Common::ScopedHeap & heap, FABRIC_SCALING_MECHANISM &) const;

        virtual Common::ErrorCode Validate(bool isStateful) const;

        static Serialization::IFabricSerializable * FabricSerializerActivator(
            Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(
            __out Serialization::FabricTypeInformation & typeInformation) const;

        static HRESULT CheckScalingMechanism(std::shared_ptr<ScalingMechanism> *);

        static ScalingMechanismSPtr CreateSPtr(ScalingMechanismKind::Enum kind);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
        END_DYNAMIC_SIZE_ESTIMATION()

        virtual void ReadFromXml(Common::XmlReaderUPtr const & xmlReader) { xmlReader; }
        virtual Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter) { xmlWriter; return Common::ErrorCodeValue::Success; }

        JSON_TYPE_ACTIVATOR_METHOD(ScalingMechanism, ScalingMechanismKind::Enum, kind_, CreateSPtr);

        FABRIC_FIELDS_01(kind_);
    protected:
        ScalingMechanismKind::Enum kind_;
    };
}
