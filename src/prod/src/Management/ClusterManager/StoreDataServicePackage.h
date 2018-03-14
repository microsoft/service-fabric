// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataServicePackage : public Store::StoreData
        {
        public:
            StoreDataServicePackage();

            StoreDataServicePackage(
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &);

            StoreDataServicePackage(
                ServiceModelApplicationId const &,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                ServiceModelPackageName const &,
                ServiceModelVersion const &,
                ServiceModel::ServiceTypeDescription const &);

            __declspec(property(get=get_ApplicationId)) ServiceModelApplicationId const & ApplicationId;
            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get=get_TypeName)) ServiceModelTypeName const & TypeName;
            __declspec(property(get=get_PackageName)) ServiceModelPackageName const & PackageName;
            __declspec(property(get=get_PackageVersion)) ServiceModelVersion const & PackageVersion;
            __declspec(property(get=get_TypeDescription)) ServiceModel::ServiceTypeDescription const & TypeDescription;
            __declspec(property(get=get_MutableTypeDescription)) ServiceModel::ServiceTypeDescription & MutableTypeDescription;

            ServiceModelApplicationId const & get_ApplicationId() const { return applicationId_; }
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            ServiceModelTypeName const & get_TypeName() const { return typeName_; }
            ServiceModelPackageName const & get_PackageName() const { return packageName_; }
            ServiceModelVersion const & get_PackageVersion() const { return packageVersion_; }
            ServiceModel::ServiceTypeDescription const & get_TypeDescription() const { return typeDescription_; }
            ServiceModel::ServiceTypeDescription & get_MutableTypeDescription() { return typeDescription_; }

            static Common::ErrorCode ReadServicePackages(
                Store::StoreTransaction const &, 
                ServiceModelApplicationId const &, 
                __out std::vector<StoreDataServicePackage> &);

            virtual ~StoreDataServicePackage() { }
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_06(applicationId_, applicationName_, typeName_, packageName_, packageVersion_, typeDescription_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            ServiceModelApplicationId applicationId_;
            // store name for debugging
            Common::NamingUri applicationName_;
            ServiceModelTypeName typeName_;
            ServiceModelPackageName packageName_;
            ServiceModelVersion packageVersion_;
            ServiceModel::ServiceTypeDescription typeDescription_;
        };
    }
}
