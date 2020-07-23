// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ContainerEndpointDescription
        : public ModelType
        {
        public:
            ContainerEndpointDescription() = default;

            bool operator==(ContainerEndpointDescription const &) const;

            DEFAULT_MOVE_ASSIGNMENT(ContainerEndpointDescription)
            DEFAULT_MOVE_CONSTRUCTOR(ContainerEndpointDescription)
            DEFAULT_COPY_ASSIGNMENT(ContainerEndpointDescription)
            DEFAULT_COPY_CONSTRUCTOR(ContainerEndpointDescription)

            __declspec(property(get=get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::nameCamelCase, name_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::port, port_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            Common::ErrorCode TryValidate(std::wstring const &traceId) const override;

            FABRIC_FIELDS_02(name_, port_);
            
            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(port_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(useDynamicHostPort_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            std::wstring name_;
            int port_;
            bool useDynamicHostPort_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::ContainerEndpointDescription);
