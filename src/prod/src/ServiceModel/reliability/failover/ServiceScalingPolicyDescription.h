//-----------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

namespace Reliability
{
    struct ServiceScalingPolicyDescription;
    using ServiceScalingPolicyDescriptionSPtr = std::shared_ptr<ServiceScalingPolicyDescription>;

    struct ServiceScalingPolicyDescription
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        ServiceScalingPolicyDescription();
        ServiceScalingPolicyDescription(ScalingTriggerSPtr, ScalingMechanismSPtr);

        ServiceScalingPolicyDescription(ServiceScalingPolicyDescription const & other) = default;
        ServiceScalingPolicyDescription & operator=(ServiceScalingPolicyDescription const & other) = default;
        ServiceScalingPolicyDescription(ServiceScalingPolicyDescription && other) = default;
        ServiceScalingPolicyDescription & operator=(ServiceScalingPolicyDescription && other) = default;

        bool operator == (ServiceScalingPolicyDescription const & other) const;
        bool operator != (ServiceScalingPolicyDescription const & other) const;

        virtual Common::ErrorCode Equals(
            ServiceScalingPolicyDescription const & other,
            bool ignoreDynamicContent) const;

        virtual ~ServiceScalingPolicyDescription() = default;

        __declspec(property(get = get_Mechanism)) ScalingMechanismSPtr const& Mechanism;
        ScalingMechanismSPtr get_Mechanism() const { return mechanism_; }

        __declspec(property(get = get_Trigger)) ScalingTriggerSPtr const& Trigger;
        ScalingTriggerSPtr get_Trigger() const { return trigger_; }

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        // True if auto scaling is done by changing instance count of a single partition.
        bool IsPartitionScaled() const { return mechanism_ != nullptr && mechanism_->Kind == ScalingMechanismKind::PartitionInstanceCount; }

        // True is auto scaling is done by adding or removing partitions (in any way)
        bool IsServiceScaled() const { return mechanism_ != nullptr && mechanism_->Kind == ScalingMechanismKind::AddRemoveIncrementalNamedPartition; }

        Common::TimeSpan GetScalingInterval() const;

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SERVICE_SCALING_POLICY & scalingPolicy) const;

        Common::ErrorCode FromPublicApi(
            __in FABRIC_SERVICE_SCALING_POLICY const & scalingPolicy);

        Common::ErrorCode Validate(bool isStateful) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ScalingTrigger, trigger_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ScalingMechanism, mechanism_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(mechanism_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(trigger_)
        END_DYNAMIC_SIZE_ESTIMATION()

        void ReadFromXml(Common::XmlReaderUPtr const & xmlReader);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);

        FABRIC_FIELDS_02(trigger_, mechanism_)
    protected:
        ScalingMechanismSPtr mechanism_;
        ScalingTriggerSPtr trigger_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::ServiceScalingPolicyDescription);