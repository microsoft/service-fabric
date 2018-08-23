// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    struct ScalingTrigger;
    using ScalingTriggerSPtr = std::shared_ptr<ScalingTrigger>;

    struct ScalingTrigger
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ScalingTrigger();

        ScalingTrigger(ScalingTriggerKind::Enum kind);

        ScalingTrigger(ScalingTrigger const & other) = default;
        ScalingTrigger & operator=(ScalingTrigger const & other) = default;
        ScalingTrigger(ScalingTrigger && other) = default;
        ScalingTrigger & operator=(ScalingTrigger && other) = default;

        virtual ~ScalingTrigger() = default;

        virtual bool Equals(ScalingTriggerSPtr const & other, bool ignoreDynamicContent) const;

        __declspec (property(get = get_Kind)) ScalingTriggerKind::Enum Kind;
        ScalingTriggerKind::Enum get_Kind() const { return kind_; }

        // In case when trigger is based on a metric, this will return metric name.
        // In other cases it will return an empty string.
        virtual std::wstring GetMetricName() const { return L""; }

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        virtual Common::ErrorCode FromPublicApi(FABRIC_SCALING_TRIGGER const & policy);

        virtual void ToPublicApi(Common::ScopedHeap & heap, FABRIC_SCALING_TRIGGER &) const;

        virtual Common::ErrorCode Validate(bool isStateful) const;

        static Serialization::IFabricSerializable * FabricSerializerActivator(
            Serialization::FabricTypeInformation typeInformation);

        virtual NTSTATUS GetTypeInformation(
            __out Serialization::FabricTypeInformation & typeInformation) const;

        static HRESULT CheckScalingTrigger(std::shared_ptr<ScalingTrigger> *);

        static ScalingTriggerSPtr CreateSPtr(ScalingTriggerKind::Enum kind);

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
        END_DYNAMIC_SIZE_ESTIMATION()

        virtual void ReadFromXml(Common::XmlReaderUPtr const & xmlReader) { xmlReader; }
        virtual Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter) { xmlWriter; return Common::ErrorCodeValue::Success; }

        JSON_TYPE_ACTIVATOR_METHOD(ScalingTrigger, ScalingTriggerKind::Enum, kind_, CreateSPtr);

        FABRIC_FIELDS_01(kind_);
    protected:
        ScalingTriggerKind::Enum kind_;
    };
}
