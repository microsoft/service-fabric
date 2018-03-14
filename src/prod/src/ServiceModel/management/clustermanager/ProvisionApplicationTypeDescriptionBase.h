// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ProvisionApplicationTypeDescriptionBase;
        using ProvisionApplicationTypeDescriptionBaseSPtr = std::shared_ptr<ProvisionApplicationTypeDescriptionBase>;

        class ProvisionApplicationTypeDescriptionBase
            : public Common::IFabricJsonSerializable
        {
            DENY_COPY(ProvisionApplicationTypeDescriptionBase)
        
        public:
            ProvisionApplicationTypeDescriptionBase();

            explicit ProvisionApplicationTypeDescriptionBase(FABRIC_PROVISION_APPLICATION_TYPE_KIND kind);
            
            ProvisionApplicationTypeDescriptionBase(ProvisionApplicationTypeDescriptionBase && other) = default;
            ProvisionApplicationTypeDescriptionBase & operator = (ProvisionApplicationTypeDescriptionBase && other) = default;

            virtual ~ProvisionApplicationTypeDescriptionBase();

            __declspec(property(get = get_Kind)) FABRIC_PROVISION_APPLICATION_TYPE_KIND Kind;
            FABRIC_PROVISION_APPLICATION_TYPE_KIND get_Kind() const { return kind_; }

            __declspec(property(get=get_IsAsync)) bool IsAsync;
            bool get_IsAsync() const { return isAsync_; }

            static ProvisionApplicationTypeDescriptionBaseSPtr CreateSPtr(FABRIC_PROVISION_APPLICATION_TYPE_KIND kind);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::Kind, kind_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Async, isAsync_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            JSON_TYPE_ACTIVATOR_METHOD(ProvisionApplicationTypeDescriptionBase, FABRIC_PROVISION_APPLICATION_TYPE_KIND, kind_, CreateSPtr)

        protected:
            Common::ErrorCode TraceAndGetError(
                Common::ErrorCodeValue::Enum errorValue,
                std::wstring && message) const;

        protected:
            FABRIC_PROVISION_APPLICATION_TYPE_KIND kind_;
            bool isAsync_;
        };

        template<FABRIC_PROVISION_APPLICATION_TYPE_KIND Kind>
        class ProvisionApplicationTypeSerializationTypeActivator
        {
        public:
            static ProvisionApplicationTypeDescriptionBaseSPtr CreateSPtr()
            {
                return std::make_shared<ProvisionApplicationTypeDescriptionBase>(FABRIC_PROVISION_APPLICATION_TYPE_KIND_INVALID);
            }
        };

        // template specializations, to be defined in derived classes
    #define DEFINE_PROVISION_APPLICATION_TYPE_ACTIVATOR(TYPE_SERVICEMODEL, IDL_ENUM)                      \
        template<> class ProvisionApplicationTypeSerializationTypeActivator<IDL_ENUM>                     \
        {                                                                                                 \
        public:                                                                                           \
            static ProvisionApplicationTypeDescriptionBaseSPtr CreateSPtr()                               \
            {                                                                                             \
                return std::make_shared<TYPE_SERVICEMODEL>();                                             \
            }                                                                                             \
        };
    }
}
