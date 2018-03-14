// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataServiceManifest : public Store::StoreData
        {
        public:
            StoreDataServiceManifest();

            StoreDataServiceManifest(
                ServiceModelTypeName const &,
                ServiceModelVersion const &);

            StoreDataServiceManifest(
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::vector<ServiceModelServiceManifestDescription> && );

            void TrimDuplicateManifestsAndPackages(std::vector<StoreDataServiceManifest> const &);
            std::wstring GetTypeNameKeyPrefix() const;

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const;
        
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

             __declspec(property(get=get_ApplicationTypeName)) ServiceModelTypeName const & ApplicationTypeName;
            ServiceModelTypeName const & get_ApplicationTypeName() const { return applicationTypeName_; }
            
            __declspec(property(get=get_ApplicationTypeVersion)) ServiceModelVersion const & ApplicationTypeVersion;
            ServiceModelVersion const & get_ApplicationTypeVersion() const { return applicationTypeVersion_; }

            __declspec(property(put=set_ServiceManifests, get=get_ServiceManifests)) std::vector<ServiceModelServiceManifestDescription> const & ServiceManifests;
            std::vector<ServiceModelServiceManifestDescription> const & get_ServiceManifests() const { return serviceManifests_; }
            void set_ServiceManifests(std::vector<ServiceModelServiceManifestDescription> const && manifests) { serviceManifests_ = move(manifests); }

            FABRIC_FIELDS_03(applicationTypeName_, applicationTypeVersion_, serviceManifests_);

        protected:            
            virtual std::wstring ConstructKey() const;

        private:
            ServiceModelTypeName applicationTypeName_;
            ServiceModelVersion applicationTypeVersion_;
            std::vector<ServiceModelServiceManifestDescription> serviceManifests_;
        };
    }
}
