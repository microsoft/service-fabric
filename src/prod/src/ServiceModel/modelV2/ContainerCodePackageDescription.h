//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ContainerCodePackageDescription
            : public ModelType
            , public Common::ISizeEstimator
        {
        public:
            __declspec(property(get=get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec(property(get=get_VolumeRefs)) std::vector<IndependentVolumeRef> & VolumeRefs;
            std::vector<IndependentVolumeRef> & get_VolumeRefs() { return volumeRefs_; }

            __declspec(property(get=get_EndpointRefs)) std::vector<ContainerEndpointDescription> const & EndpointRefs;
            std::vector<ContainerEndpointDescription> const & get_EndpointRefs() const { return endpoints_; }

            __declspec(property(get=get_InstanceViewPtr, put=put_InstanceViewPtr)) std::shared_ptr<CodePackageInstanceView> & InstanceViewSPtr;
            std::shared_ptr<CodePackageInstanceView> & get_InstanceViewPtr() { return instanceViewPtr_; }
            void put_InstanceViewPtr(std::shared_ptr<CodePackageInstanceView> && value) { instanceViewPtr_ = move(value); }

            bool operator==(ContainerCodePackageDescription const & other) const;

            bool CanUpgrade(ContainerCodePackageDescription const & other) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::nameCamelCase, name_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::image, image_)
                SERIALIZABLE_PROPERTY(L"imageRegistryCredential", imageRegistryCredential_)
                SERIALIZABLE_PROPERTY_IF(L"entryPoint", entryPoint_, !entryPoint_.empty())
                SERIALIZABLE_PROPERTY_IF(L"commands", commands_, !commands_.empty())
                SERIALIZABLE_PROPERTY_IF(L"reliableCollectionsRefs", reliableCollectionsRefs_, reliableCollectionsRefs_.size() != 0) // TEMPORARY -- for //BUILD demo
                SERIALIZABLE_PROPERTY_IF(L"volumeRefs", volumeRefs_, !volumeRefs_.empty())
                SERIALIZABLE_PROPERTY_IF(L"volumes", volumes_, !volumes_.empty())
                SERIALIZABLE_PROPERTY_IF(L"endpoints", endpoints_, !endpoints_.empty())
                SERIALIZABLE_PROPERTY_IF(ServiceModel::Constants::environmentVariables, environmentVariables_, !environmentVariables_.empty())
                SERIALIZABLE_PROPERTY_IF(L"settings", settings_, !settings_.empty())
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::resources, resources_)
                SERIALIZABLE_PROPERTY_IF(L"instanceView", instanceViewPtr_, instanceViewPtr_ != nullptr)
                SERIALIZABLE_PROPERTY_IF(L"diagnostics", diagnostics_, diagnostics_ != nullptr)
                SERIALIZABLE_PROPERTY_IF(L"labels", labels_, !labels_.empty())
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_15(name_, image_, imageRegistryCredential_, entryPoint_, commands_, volumeRefs_, volumes_, endpoints_, environmentVariables_, resources_, instanceViewPtr_, settings_, diagnostics_, reliableCollectionsRefs_, labels_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(name_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(image_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(imageRegistryCredential_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(entryPoint_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(commands_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(endpoints_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(reliableCollectionsRefs_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(environmentVariables_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(settings_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(instanceViewPtr_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(resources_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(volumeRefs_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(volumes_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(diagnostics_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(labels_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const &traceId) const override;

        private:
            std::wstring name_;
            std::wstring image_;
            ImageRegistryCredential imageRegistryCredential_;
            std::wstring entryPoint_;
            std::vector<std::wstring> commands_;
            std::vector<ReliableCollectionsRef> reliableCollectionsRefs_;
            std::vector<IndependentVolumeRef> volumeRefs_;
            std::vector<ApplicationScopedVolume> volumes_;
            std::vector<ContainerEndpointDescription> endpoints_;
            std::vector<SettingDescription> settings_;
            std::vector<ServiceModel::EnvironmentVariableDescription> environmentVariables_;
            CodePackageResourceDescription resources_;
            std::shared_ptr<CodePackageInstanceView> instanceViewPtr_;
            std::shared_ptr<DiagnosticsRef> diagnostics_;
            std::vector<ContainerLabel> labels_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ModelV2::ContainerCodePackageDescription);
