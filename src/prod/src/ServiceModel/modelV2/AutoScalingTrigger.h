// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel 
{
    namespace ModelV2
    {
        struct AutoScalingTrigger;
        using AutoScalingTriggerSPtr = std::shared_ptr<AutoScalingTrigger>;

        struct AutoScalingTrigger
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            AutoScalingTrigger();

            AutoScalingTrigger(AutoScalingTriggerKind::Enum kind);

            DEFAULT_MOVE_ASSIGNMENT(AutoScalingTrigger)
            DEFAULT_MOVE_CONSTRUCTOR(AutoScalingTrigger)
            DEFAULT_COPY_ASSIGNMENT(AutoScalingTrigger)
            DEFAULT_COPY_CONSTRUCTOR(AutoScalingTrigger)

            bool operator==(AutoScalingTriggerSPtr const & other) const;

            virtual bool Equals(AutoScalingTriggerSPtr const & other, bool ignoreDynamicContent) const;

            virtual ~AutoScalingTrigger() = default;

            __declspec (property(get = get_Kind)) AutoScalingTriggerKind::Enum Kind;
            AutoScalingTriggerKind::Enum get_Kind() const { return kind_; }

            // In case when trigger is based on a metric, this will return metric name.
            // In other cases it will return an empty string.
            virtual std::wstring GetMetricName() const { return L""; }

            virtual Common::ErrorCode Validate() const;

            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            static AutoScalingTriggerSPtr CreateSPtr(AutoScalingTriggerKind::Enum kind);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
            END_DYNAMIC_SIZE_ESTIMATION()

            JSON_TYPE_ACTIVATOR_METHOD(AutoScalingTrigger, AutoScalingTriggerKind::Enum, kind_, CreateSPtr);

            FABRIC_FIELDS_01(kind_);

        protected:
            AutoScalingTriggerKind::Enum kind_;
        };
    }
}