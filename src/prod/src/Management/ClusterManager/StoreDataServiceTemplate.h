// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class StoreDataServiceTemplate : public Store::StoreData
        {
        public:
            StoreDataServiceTemplate();

            StoreDataServiceTemplate(
                ServiceModelApplicationId const &,
                ServiceModelTypeName const &);

            StoreDataServiceTemplate(
                ServiceModelApplicationId const &,
                Common::NamingUri const &,
                ServiceModelTypeName const &,
                Naming::PartitionedServiceDescriptor &&);

            __declspec(property(get=get_ApplicationId)) ServiceModelApplicationId const & ApplicationId;
            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get=get_TypeName)) ServiceModelTypeName const & TypeName;
            __declspec(property(get=get_PartitionedServiceDescriptor)) Naming::PartitionedServiceDescriptor const & PartitionedServiceDescriptor;
            __declspec(property(get=get_MutablePartitionedServiceDescriptor)) Naming::PartitionedServiceDescriptor & MutablePartitionedServiceDescriptor;

            ServiceModelApplicationId const & get_ApplicationId() const { return applicationId_; }
            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            ServiceModelTypeName const & get_TypeName() const { return serviceTypeName_; }
            Naming::PartitionedServiceDescriptor const & get_PartitionedServiceDescriptor() const { return partitionedServiceDescriptor_; }
            Naming::PartitionedServiceDescriptor & get_MutablePartitionedServiceDescriptor() { return partitionedServiceDescriptor_; }

            static Common::ErrorCode ReadServiceTemplates(
                Store::StoreTransaction const &, 
                ServiceModelApplicationId const &, 
                __out std::vector<StoreDataServiceTemplate> &);

            virtual ~StoreDataServiceTemplate() { }
            virtual std::wstring const & get_Type() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_04(applicationId_, applicationName_, serviceTypeName_, partitionedServiceDescriptor_);

        protected:
            virtual std::wstring ConstructKey() const;

        private:
            ServiceModelApplicationId applicationId_;
            // store name for debugging
            Common::NamingUri applicationName_;
            ServiceModelTypeName serviceTypeName_;
            Naming::PartitionedServiceDescriptor partitionedServiceDescriptor_;
        };
    }
}
