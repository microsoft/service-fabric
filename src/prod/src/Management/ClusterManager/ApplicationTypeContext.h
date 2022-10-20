// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ApplicationTypeContext : public RolloutContext
        {
        public:
            static const RolloutContextType::Enum ContextType;

            ApplicationTypeContext();

            ApplicationTypeContext(
                ServiceModelTypeName const &,
                ServiceModelVersion const &);

            ApplicationTypeContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                std::wstring const & buildPath,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                bool isAsync,
                std::wstring const & applicationPackageDownloadUri);

            ApplicationTypeContext(
                Common::ComponentRoot const &,
                ClientRequestSPtr const &,
                std::wstring const & buildPath,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                bool isAsync,
                std::wstring const & applicationPackageDownloadUri,
                ApplicationPackageCleanupPolicy::Enum applicationPackageCleanupPolicy);

            ApplicationTypeContext(
                Store::ReplicaActivityId const &,
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                bool isAsync,
                ApplicationTypeDefinitionKind::Enum const definitionKind,
                std::wstring const &);
            
            ApplicationTypeContext(ApplicationTypeContext &&) = default;
            ApplicationTypeContext(ApplicationTypeContext const &) = default;
            ApplicationTypeContext & operator=(ApplicationTypeContext &&) = default;

            virtual ~ApplicationTypeContext();

            __declspec(property(get=get_BuildPath)) std::wstring const & BuildPath;
            std::wstring const & get_BuildPath() const { return buildPath_; }

            __declspec(property(get=get_TypeName)) ServiceModelTypeName const & TypeName;
            ServiceModelTypeName const & get_TypeName() const { return typeName_; }

            __declspec(property(get=get_TypeVersion)) ServiceModelVersion const & TypeVersion;
            ServiceModelVersion const & get_TypeVersion() const { return typeVersion_; }

            __declspec(property(get = get_ManifestId, put = set_ManifestId)) std::wstring const & ManifestId;
            std::wstring const & get_ManifestId() const { return manifestId_; }
            void set_ManifestId(std::wstring const & value) { manifestId_ = value; }

            __declspec(property(get=get_IsAsync)) bool IsAsync;
            bool get_IsAsync() const { return isAsync_; }

            __declspec(property(get=get_QueryStatus)) ServiceModel::ApplicationTypeStatus::Enum QueryStatus;
            ServiceModel::ApplicationTypeStatus::Enum get_QueryStatus() const;

            __declspec(property(get=get_QueryStatusDetails, put=set_QueryStatusDetails)) std::wstring const & QueryStatusDetails;
            std::wstring const & get_QueryStatusDetails() const { return statusDetails_; }
            void set_QueryStatusDetails(std::wstring const & value) { statusDetails_ = value; }

            __declspec(property(get=get_manifestInStore, put=set_manifestInStore)) bool const & ManifestDataInStore;
            bool const & get_manifestInStore() const { return manifestDataInStore_; }
            void set_manifestInStore(bool data) { manifestDataInStore_ = data; }

            __declspec(property(get=get_ApplicationTypeDefinitionKind)) ApplicationTypeDefinitionKind::Enum ApplicationTypeDefinitionKind;
            ApplicationTypeDefinitionKind::Enum get_ApplicationTypeDefinitionKind() const { return applicationTypeDefinitionKind_; }
            void UpdateIsAsync(bool value) { isAsync_ = value; }

            __declspec(property(get = get_ApplicationPackageDownloadUri)) std::wstring const & ApplicationPackageDownloadUri;
            std::wstring const & get_ApplicationPackageDownloadUri() const { return applicationPackageDownloadUri_; }

            __declspec(property(get = get_ApplicationPackageCleanupPolicy)) ApplicationPackageCleanupPolicy::Enum ApplicationPackageCleanupPolicy;
            ApplicationPackageCleanupPolicy::Enum get_ApplicationPackageCleanupPolicy() const { return applicationPackageCleanupPolicy_; }

            void OnFailRolloutContext() override { } // no-op

            virtual std::wstring const & get_Type() const;
            virtual std::wstring ConstructKey() const;
            std::wstring GetTypeNameKeyPrefix() const;

            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
                
            FABRIC_FIELDS_10(
                buildPath_,
                typeName_,
                typeVersion_,
                manifestDataInStore_,
                isAsync_,
                statusDetails_,
                applicationTypeDefinitionKind_,
                manifestId_,
                applicationPackageDownloadUri_,
                applicationPackageCleanupPolicy_);

        private:
            std::wstring buildPath_;
            ServiceModelTypeName typeName_;
            ServiceModelVersion typeVersion_;
            bool manifestDataInStore_;
            bool isAsync_;
            std::wstring statusDetails_;
            ApplicationTypeDefinitionKind::Enum applicationTypeDefinitionKind_;
            std::wstring manifestId_;
            std::wstring applicationPackageDownloadUri_;
            ApplicationPackageCleanupPolicy::Enum applicationPackageCleanupPolicy_;
        };

        typedef std::shared_ptr<ApplicationTypeContext> ApplicationTypeContextSPtr;
    }
}
