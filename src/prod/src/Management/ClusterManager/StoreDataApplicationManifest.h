// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataApplicationManifest : public Store::StoreData
        {
        public:
            StoreDataApplicationManifest();

            StoreDataApplicationManifest(
                ServiceModelTypeName const &,
                ServiceModelVersion const &);

            StoreDataApplicationManifest(StoreDataApplicationManifest &&);

            StoreDataApplicationManifest(
                ServiceModelTypeName const &,
                ServiceModelVersion const &,
                std::wstring &&,
                ServiceModel::ApplicationHealthPolicy &&,
                std::map<std::wstring, std::wstring> &&);

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const;
        
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            __declspec(property(get=get_TypeName)) ServiceModelTypeName const & TypeName;
            ServiceModelTypeName const & get_TypeName() const { return typeName_; }

            __declspec(property(get=get_TypeVersion)) ServiceModelVersion const & TypeVersion;
            ServiceModelVersion const & get_TypeVersion() const { return typeVersion_; }

            __declspec(property(get=get_ApplicationManifest)) std::wstring const & ApplicationManifest;
            std::wstring const & get_ApplicationManifest() const { return applicationManifest_; }

            __declspec(property(get=get_HealthPolicy)) ServiceModel::ApplicationHealthPolicy const & HealthPolicy;
            ServiceModel::ApplicationHealthPolicy const & get_HealthPolicy() const { return healthPolicy_; }

            __declspec(property(get=get_DefaultParameters))std::map<std::wstring, std::wstring> const & DefaultParameters;
            std::map<std::wstring, std::wstring> const& get_DefaultParameters() const { return applicationParameters_; } 

            std::map<std::wstring, std::wstring> GetMergedApplicationParameters(std::map<std::wstring, std::wstring> const &) const;

            FABRIC_FIELDS_05(typeName_, typeVersion_, applicationManifest_, healthPolicy_, applicationParameters_);

            virtual std::wstring ConstructKey() const;

        private:
            ServiceModelTypeName typeName_;
            ServiceModelVersion typeVersion_;
            std::wstring applicationManifest_;
            ServiceModel::ApplicationHealthPolicy healthPolicy_;
            std::map<std::wstring, std::wstring> applicationParameters_;
        };
    }
}
