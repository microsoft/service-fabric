// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        struct AddRemoveReplicaScalingMechanism;
        using AddRemoveReplicaScalingMechanismSPtr = std::shared_ptr<AddRemoveReplicaScalingMechanism>;

        struct AddRemoveReplicaScalingMechanism : public AutoScalingMechanism
        {
        public:
            AddRemoveReplicaScalingMechanism();

            AddRemoveReplicaScalingMechanism(
                int minCount,
                int maxCount,
                int scaleIncrement);

            DEFAULT_MOVE_ASSIGNMENT(AddRemoveReplicaScalingMechanism)
            DEFAULT_MOVE_CONSTRUCTOR(AddRemoveReplicaScalingMechanism)
            DEFAULT_COPY_ASSIGNMENT(AddRemoveReplicaScalingMechanism)
            DEFAULT_COPY_CONSTRUCTOR(AddRemoveReplicaScalingMechanism)

            virtual ~AddRemoveReplicaScalingMechanism() = default;

            virtual bool Equals(AutoScalingMechanismSPtr const & other, bool ignoreDynamicContent) const;

            __declspec(property(get = get_MinCount)) int MinCount;
            int get_MinCount() const { return minCount_; }

            __declspec(property(get = get_MaxCount)) int MaxCount;
            int get_MaxCount() const { return maxCount_; }

            __declspec(property(get = get_ScaleIncrement)) int ScaleIncrement;
            int get_ScaleIncrement() const { return scaleIncrement_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingMinInstanceCount, minCount_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingMaxInstanceCount, maxCount_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScaleIncrement, scaleIncrement_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            virtual Common::ErrorCode Validate() const;

            FABRIC_FIELDS_03(minCount_, maxCount_, scaleIncrement_);
        private:
            int minCount_;
            int maxCount_;
            int scaleIncrement_;
        };
    }
}