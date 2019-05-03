// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        struct AutoScalingMechanism;
        using AutoScalingMechanismSPtr = std::shared_ptr<AutoScalingMechanism>;

        struct AutoScalingMechanism
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            AutoScalingMechanism();

            AutoScalingMechanism(AutoScalingMechanismKind::Enum kind);

            bool operator==(AutoScalingMechanismSPtr const & other) const;

            virtual bool Equals(AutoScalingMechanismSPtr const & other, bool ignoreDynamicContent) const;

            virtual ~AutoScalingMechanism() = default;

            __declspec (property(get = get_Kind)) AutoScalingMechanismKind::Enum Kind;
            AutoScalingMechanismKind::Enum get_Kind() const { return kind_; }

            virtual Common::ErrorCode Validate() const;

            static Serialization::IFabricSerializable * FabricSerializerActivator(
                Serialization::FabricTypeInformation typeInformation);

            virtual NTSTATUS GetTypeInformation(
                __out Serialization::FabricTypeInformation & typeInformation) const;

            DEFAULT_MOVE_ASSIGNMENT(AutoScalingMechanism)
            DEFAULT_MOVE_CONSTRUCTOR(AutoScalingMechanism)
            DEFAULT_COPY_ASSIGNMENT(AutoScalingMechanism)
            DEFAULT_COPY_CONSTRUCTOR(AutoScalingMechanism)

            static AutoScalingMechanismSPtr CreateSPtr(AutoScalingMechanismKind::Enum kind);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::autoScalingMechanismKind, kind_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
            END_DYNAMIC_SIZE_ESTIMATION()

            JSON_TYPE_ACTIVATOR_METHOD(AutoScalingMechanism, AutoScalingMechanismKind::Enum, kind_, CreateSPtr);

            FABRIC_FIELDS_01(kind_);

        protected:
            AutoScalingMechanismKind::Enum kind_;
        };
    }
}