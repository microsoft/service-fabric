// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    struct AddRemoveIncrementalNamedPartitionScalingMechanism;
    using AddRemoveIncrementalNamedPartitionScalingMechanismSPtr = std::shared_ptr<AddRemoveIncrementalNamedPartitionScalingMechanism>;

    struct AddRemoveIncrementalNamedPartitionScalingMechanism : public ScalingMechanism
    {
    public:
        AddRemoveIncrementalNamedPartitionScalingMechanism();

        AddRemoveIncrementalNamedPartitionScalingMechanism(int minPartitionCount, int maxPartitionCount, int scaleIncrement);

        AddRemoveIncrementalNamedPartitionScalingMechanism(AddRemoveIncrementalNamedPartitionScalingMechanism const & other) = default;
        AddRemoveIncrementalNamedPartitionScalingMechanism & operator=(AddRemoveIncrementalNamedPartitionScalingMechanism const & other) = default;
        AddRemoveIncrementalNamedPartitionScalingMechanism(AddRemoveIncrementalNamedPartitionScalingMechanism && other) = default;
        AddRemoveIncrementalNamedPartitionScalingMechanism & operator=(AddRemoveIncrementalNamedPartitionScalingMechanism && other) = default;

        virtual ~AddRemoveIncrementalNamedPartitionScalingMechanism() = default;

        virtual bool Equals(ScalingMechanismSPtr const & other, bool ignoreDynamicContent) const;

        __declspec(property(get = get_MinimumPartitionCount)) int MinimumPartitionCount;
        int get_MinimumPartitionCount() const { return minimumPartitionCount_; }

        __declspec(property(get = get_MaximumPartitionCount)) int MaximumPartitionCount;
        int get_MaximumPartitionCount() const { return maximumPartitionCount_; }

        __declspec(property(get = get_ScaleIncrement)) int ScaleIncrement;
        int get_ScaleIncrement() const { return scaleIncrement_; }

        virtual Common::ErrorCode Validate(bool isStateful) const;

        virtual void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_CHAIN()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::MinPartitionCount, minimumPartitionCount_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::MaxPartitionCount, maximumPartitionCount_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ScaleIncrement, scaleIncrement_)
        END_JSON_SERIALIZABLE_PROPERTIES()

        virtual void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_SCALING_MECHANISM & scalingPolicy) const;
        virtual Common::ErrorCode FromPublicApi(
            FABRIC_SCALING_MECHANISM const &);

        virtual void ReadFromXml(Common::XmlReaderUPtr const & xmlReader);
        virtual Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);

        FABRIC_FIELDS_03(maximumPartitionCount_, minimumPartitionCount_, scaleIncrement_);
    private:
        int maximumPartitionCount_;
        int minimumPartitionCount_;
        int scaleIncrement_;
    };
}
