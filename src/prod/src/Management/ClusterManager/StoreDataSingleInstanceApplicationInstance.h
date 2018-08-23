//------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//------------------------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataSingleInstanceApplicationInstance : public Store::StoreData
        {
        public:
            StoreDataSingleInstanceApplicationInstance() = default;

            StoreDataSingleInstanceApplicationInstance(
                std::wstring const &,
                ServiceModelVersion const &);

            StoreDataSingleInstanceApplicationInstance(
                std::wstring const &,
                ServiceModelVersion const &,
                ServiceModel::ModelV2::ApplicationDescription const &);

            __declspec (property(get=get_Type)) std::wstring const & Type;
            virtual std::wstring const & get_Type() const;

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const override;

            std::wstring GetDeploymentNameKeyPrefix() const;

            FABRIC_FIELDS_03(
                DeploymentName,
                TypeVersion,
                ApplicationDescription);

        protected:
            virtual std::wstring ConstructKey() const override;

        public:
            std::wstring DeploymentName;
            ServiceModelVersion TypeVersion;
            ServiceModel::ModelV2::ApplicationDescription ApplicationDescription;
        };
    }
}
