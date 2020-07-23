//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class AutoScalingPolicy
            : public ModelType
        {
        public:
            __declspec(property(get=get_AutoScalingPolicyName)) std::wstring const & AutoScalingPolicyName;
            std::wstring & get_AutoScalingPolicyName() { return autoScalingName_; }

            bool operator == (AutoScalingPolicy const & other) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingName, autoScalingName_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingTrigger, autoScalingTrigger_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingMechanism, autoScalingMechanism_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_03(autoScalingName_, autoScalingTrigger_, autoScalingMechanism_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(autoScalingName_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(autoScalingTrigger_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(autoScalingMechanism_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const &traceId) const override;

        private:
            std::wstring autoScalingName_;
            AutoScalingTriggerSPtr autoScalingTrigger_;
            AutoScalingMechanismSPtr autoScalingMechanism_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::AutoScalingPolicy);
