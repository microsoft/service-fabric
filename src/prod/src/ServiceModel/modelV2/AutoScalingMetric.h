// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        struct AutoScalingMetric;
        using AutoScalingMetricSPtr = std::shared_ptr<AutoScalingMetric>;

        struct AutoScalingMetric
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            AutoScalingMetric();

            AutoScalingMetric(AutoScalingMetricKind::Enum kind);

            DEFAULT_MOVE_ASSIGNMENT(AutoScalingMetric)
            DEFAULT_MOVE_CONSTRUCTOR(AutoScalingMetric)
            DEFAULT_COPY_ASSIGNMENT(AutoScalingMetric)
            DEFAULT_COPY_CONSTRUCTOR(AutoScalingMetric)

            bool operator==(AutoScalingMetricSPtr const & other) const;

            virtual ~AutoScalingMetric() = default;

            virtual bool Equals(AutoScalingMetricSPtr const & other, bool ignoreDynamicContent) const;

            __declspec (property(get = get_Kind)) AutoScalingMetricKind::Enum Kind;
            AutoScalingMetricKind::Enum get_Kind() const { return kind_; }

            virtual std::wstring GetName() const { return L""; }

            virtual Common::ErrorCode Validate() const;

            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            static AutoScalingMetricSPtr CreateSPtr(AutoScalingMetricKind::Enum kind);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::autoScalingMetricKind, kind_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
            END_DYNAMIC_SIZE_ESTIMATION()

            JSON_TYPE_ACTIVATOR_METHOD(AutoScalingMetric, AutoScalingMetricKind::Enum, kind_, CreateSPtr);

            FABRIC_FIELDS_01(kind_);

        protected:
            AutoScalingMetricKind::Enum kind_;
        };
    }
}