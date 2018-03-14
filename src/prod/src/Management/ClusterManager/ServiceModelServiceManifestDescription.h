// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ServiceModelServiceManifestDescription : public Serialization::FabricSerializable
        {
        public:
            typedef std::pair<std::wstring, std::wstring> NameVersionPair;
            typedef std::map<std::wstring, std::wstring> NameVersionMap;

            ServiceModelServiceManifestDescription();
            ServiceModelServiceManifestDescription(std::wstring const & name, std::wstring const & version);

            __declspec(property(get=get_Name)) std::wstring const & Name;
            std::wstring const & get_Name() const { return name_; }

            __declspec(property(get=get_Version)) std::wstring const & Version;
            std::wstring const & get_Version() const { return version_; }

             __declspec(property(get=get_Content)) std::wstring const & Content;
            std::wstring const & get_Content() const { return content_; }
            inline void SetContent(std::wstring && value) { content_ = move(value); }

            __declspec(property(get=get_ServiceTypeNames)) std::vector<ServiceModel::ServiceTypeDescription> const & ServiceTypeNames;
            std::vector<ServiceModel::ServiceTypeDescription> const & get_ServiceTypeNames() const { return serviceTypeNames_; }

            __declspec(property(get = get_ServiceGroupTypeNames)) std::vector<ServiceModel::ServiceGroupTypeDescription> const & ServiceGroupTypeNames;
            std::vector<ServiceModel::ServiceGroupTypeDescription> const & get_ServiceGroupTypeNames() const { return serviceGroupTypeNames_; }

            __declspec(property(get=get_CodePackages)) NameVersionMap const & CodePackages;
            NameVersionMap const & get_CodePackages() const { return codePackages_.Map; }

            __declspec(property(get=get_ConfigPackages)) NameVersionMap const & ConfigPackages;
            NameVersionMap const & get_ConfigPackages() const { return configPackages_.Map; }

            __declspec(property(get=get_DataPackages)) NameVersionMap const & DataPackages;
            NameVersionMap const & get_DataPackages() const { return dataPackages_.Map; }

            void AddServiceModelType(ServiceModel::ServiceTypeDescription const & serviceModelTypeName);
            void AddServiceGroupModelType(ServiceModel::ServiceGroupTypeDescription const & serviceGroupModelTypeName);
            void AddCodePackage(std::wstring const & name, std::wstring const & version);
            void AddConfigPackage(std::wstring const & name, std::wstring const & version);
            void AddDataPackage(std::wstring const & name, std::wstring const & version);

            bool HasCodePackage(std::wstring const & name, std::wstring const & version);
            bool HasConfigPackage(std::wstring const & name, std::wstring const & version);
            bool HasDataPackage(std::wstring const & name, std::wstring const & version);

            void TrimDuplicatePackages(ServiceModelServiceManifestDescription const & other);
            std::vector<std::wstring> GetFilenames(Management::ImageModel::StoreLayoutSpecification const &, ServiceModelTypeName const &) const;

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            // Adding the newly added properties at the end so that backward compatibility does not break
            FABRIC_FIELDS_08(name_, version_, codePackages_, configPackages_, dataPackages_, serviceTypeNames_, content_, serviceGroupTypeNames_);

        private:
            class PackageMap : public Serialization::FabricSerializable
            {
            public:
                PackageMap() : map_() { }

                __declspec(property(get=get_Map)) NameVersionMap const & Map;
                NameVersionMap const & get_Map() const { return map_; }

                void AddPackage(std::wstring const & name, std::wstring const & version);
                bool HasPackage(std::wstring const & name, std::wstring const & version);
                void TrimDuplicatePackages(PackageMap const & other);

                void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

                FABRIC_FIELDS_01(map_);

            private:
                NameVersionMap map_;
            };

            std::wstring name_;
            std::wstring version_;
            std::wstring content_;
            std::vector<ServiceModel::ServiceTypeDescription> serviceTypeNames_;
            std::vector<ServiceModel::ServiceGroupTypeDescription> serviceGroupTypeNames_;
            PackageMap codePackages_;
            PackageMap configPackages_;
            PackageMap dataPackages_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::ClusterManager::ServiceModelServiceManifestDescription);
