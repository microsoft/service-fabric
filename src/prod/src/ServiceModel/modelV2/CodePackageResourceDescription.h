//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class CodePackageResources
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
        public:
            CodePackageResources() = default;

            bool operator==(CodePackageResources const & other) const;

            DEFAULT_MOVE_ASSIGNMENT(CodePackageResources)
            DEFAULT_MOVE_CONSTRUCTOR(CodePackageResources)
            DEFAULT_COPY_ASSIGNMENT(CodePackageResources)
            DEFAULT_COPY_CONSTRUCTOR(CodePackageResources)

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::memoryInGB, memoryInGB_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::cpu, cpu_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            __declspec(property(get=get_MemoryInGB)) double MemoryInGB;
            double get_MemoryInGB() const { return memoryInGB_; }

            __declspec(property(get=get_Cpu)) double Cpu;
            double get_Cpu() const { return cpu_; }

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(memoryInGB_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(cpu_)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_02(
                memoryInGB_,
                cpu_);

        private:
            double memoryInGB_;
            double cpu_;
        };


        class CodePackageResourceDescription
        : public ModelType
        {
        public:
            CodePackageResourceDescription() = default;

            bool operator==(CodePackageResourceDescription const & other) const;

            DEFAULT_MOVE_ASSIGNMENT(CodePackageResourceDescription)
            DEFAULT_MOVE_CONSTRUCTOR(CodePackageResourceDescription)
            DEFAULT_COPY_ASSIGNMENT(CodePackageResourceDescription)
            DEFAULT_COPY_CONSTRUCTOR(CodePackageResourceDescription)

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::resourcesRequests, resourceRequests_)
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::resourcesLimits, resourceLimitsPtr_, resourceLimitsPtr_ != nullptr)
            END_JSON_SERIALIZABLE_PROPERTIES()

            Common::ErrorCode TryValidate(std::wstring const &traceId) const override;

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(resourceRequests_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(resourceLimitsPtr_)
            END_DYNAMIC_SIZE_ESTIMATION()

            FABRIC_FIELDS_02(resourceRequests_, resourceLimitsPtr_);

        private:
            CodePackageResources resourceRequests_;
            std::shared_ptr<CodePackageResources> resourceLimitsPtr_;
        };
    }
}
