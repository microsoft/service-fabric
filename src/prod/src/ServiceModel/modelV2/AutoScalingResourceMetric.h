// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace ServiceModel
{
    namespace ModelV2
    {
        struct AutoScalingResourceMetric;
        using AutoScalingResourceMetricSPtr = std::shared_ptr<AutoScalingResourceMetric>;

        struct AutoScalingResourceMetric : public AutoScalingMetric
        {
        public:
            AutoScalingResourceMetric();

            AutoScalingResourceMetric(
                std::wstring const& name);

            DEFAULT_MOVE_ASSIGNMENT(AutoScalingResourceMetric)
            DEFAULT_MOVE_CONSTRUCTOR(AutoScalingResourceMetric)
            DEFAULT_COPY_ASSIGNMENT(AutoScalingResourceMetric)
            DEFAULT_COPY_CONSTRUCTOR(AutoScalingResourceMetric)

            virtual ~AutoScalingResourceMetric() = default;

            virtual bool Equals(AutoScalingMetricSPtr const & other, bool ignoreDynamicContent) const;

            __declspec(property(get = get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            virtual std::wstring GetName() const { return name_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::autoScalingMetricName, name_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_CHAIN()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
            END_DYNAMIC_SIZE_ESTIMATION()

            virtual Common::ErrorCode Validate() const;

            FABRIC_FIELDS_01(name_);
        private:
            std::wstring name_;
        };
    }
}