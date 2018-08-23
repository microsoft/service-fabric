// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------
#pragma once

namespace Reliability
{
    struct InstanceCountScalingMechanism;
    using InstanceCountScalingMechanismSPtr = std::shared_ptr<InstanceCountScalingMechanism>;

    struct InstanceCountScalingMechanism : public ScalingMechanism
    {
    public:
        InstanceCountScalingMechanism();

        InstanceCountScalingMechanism(
            int minimumInstanceCount,
            int maximumInstanceCount,
            int scaleIncrement);

        InstanceCountScalingMechanism(InstanceCountScalingMechanism const & other) = default;
        InstanceCountScalingMechanism & operator=(InstanceCountScalingMechanism const & other) = default;
        InstanceCountScalingMechanism(InstanceCountScalingMechanism && other) = default;
        InstanceCountScalingMechanism & operator=(InstanceCountScalingMechanism && other) = default;

        virtual ~InstanceCountScalingMechanism() = default;

        virtual bool Equals(ScalingMechanismSPtr const & other, bool ignoreDynamicContent) const;

        __declspec(property(get = get_MinimumInstanceCount)) int MinimumInstanceCount;
        int get_MinimumInstanceCount() const { return minimumInstanceCount_; }

        __declspec(property(get = get_MaximumInstanceCount)) int MaximumInstanceCount;
        int get_MaximumInstanceCount() const { return maximumInstanceCount_; }

        __declspec(property(get = get_ScaleIncrement)) int ScaleIncrement;
        int get_ScaleIncrement() const { return scaleIncrement_; }

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::MinInstanceCount, minimumInstanceCount_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxInstanceCount, maximumInstanceCount_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ScaleIncrement, scaleIncrement_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SCALING_MECHANISM & scalingPolicy) const;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_SCALING_MECHANISM const &);

        virtual Common::ErrorCode Validate(bool isStateful) const;

        virtual void ReadFromXml(Common::XmlReaderUPtr const & xmlReader);
        virtual Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);

        FABRIC_FIELDS_03(maximumInstanceCount_, minimumInstanceCount_, scaleIncrement_);
    private:
        int maximumInstanceCount_;
        int minimumInstanceCount_;
        int scaleIncrement_;
    };
}
