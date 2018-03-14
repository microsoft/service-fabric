// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class VersionedServiceTypeIdentifier : public Serialization::FabricSerializable
    {
    public:
        VersionedServiceTypeIdentifier();
        VersionedServiceTypeIdentifier(ServiceTypeIdentifier const & serviceTypeId, ServicePackageVersionInstance const & servicePackageVersionInstance);
        VersionedServiceTypeIdentifier(VersionedServiceTypeIdentifier const & other);
        VersionedServiceTypeIdentifier(VersionedServiceTypeIdentifier && other);

        __declspec(property(get=get_Id)) ServiceTypeIdentifier const & Id;
        inline ServiceTypeIdentifier const & get_Id() const { return id_; };
        
        __declspec(property(get=get_servicePackageVersionInstance)) ServicePackageVersionInstance const & servicePackageVersionInstance;
        inline ServicePackageVersionInstance const & get_servicePackageVersionInstance() const { return servicePackageVersionInstance_; };

        VersionedServiceTypeIdentifier const & operator = (VersionedServiceTypeIdentifier const & other);
        VersionedServiceTypeIdentifier const & operator = (VersionedServiceTypeIdentifier && other);

        bool operator == (VersionedServiceTypeIdentifier const & other) const;
        bool operator != (VersionedServiceTypeIdentifier const & other) const;

        int compare(VersionedServiceTypeIdentifier const & other) const;
        bool operator < (VersionedServiceTypeIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        
        FABRIC_FIELDS_02(id_, servicePackageVersionInstance_);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {   
            std::string format = "{{0},{1}}";
            size_t index = 0;

            traceEvent.AddEventField<ServiceModel::ServiceTypeIdentifier>(format, name + ".serviceTypeId", index);
            traceEvent.AddEventField<ServiceModel::ServicePackageVersionInstance>(format, name + ".servicePkgVersionInstance", index);

            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<ServiceModel::ServiceTypeIdentifier>(id_);
            context.Write<ServiceModel::ServicePackageVersionInstance>(servicePackageVersionInstance_);
        }

    private:
        ServiceTypeIdentifier id_;
        ServicePackageVersionInstance servicePackageVersionInstance_;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::VersionedServiceTypeIdentifier);
