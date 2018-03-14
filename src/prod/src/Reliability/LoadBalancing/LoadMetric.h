// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace LoadBalancingComponent
    {
        class LoadMetric : public Serialization::FabricSerializable
        {
            DENY_COPY_ASSIGNMENT(LoadMetric);

        public:
            LoadMetric() {}

            LoadMetric(std::wstring && name, uint value);

            LoadMetric(LoadMetric && other);

            LoadMetric(LoadMetric const& other);

            LoadMetric & operator = (LoadMetric && other);

            // properties
            __declspec (property(get=get_Name)) std::wstring const& Name;
            std::wstring const& get_Name() const { return name_; }

            __declspec (property(get=get_Value)) uint Value;
            uint get_Value() const { return value_; }

            void WriteTo(Common::TextWriter& writer, Common::FormatOptions const &) const;

            FABRIC_FIELDS_02(name_, value_);

        private:

            std::wstring name_;
            uint value_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Reliability::LoadBalancingComponent::LoadMetric);
