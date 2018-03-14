// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServicePackageVersionInstance : public Serialization::FabricSerializable
    {
    public:
        ServicePackageVersionInstance();
        ServicePackageVersionInstance(ServicePackageVersion const & version, uint64 instanceId);
        ServicePackageVersionInstance(ServicePackageVersionInstance const &);
        ServicePackageVersionInstance(ServicePackageVersionInstance &&);
        virtual ~ServicePackageVersionInstance();

        __declspec(property(get=get_Version)) ServiceModel::ServicePackageVersion const & Version;
        ServiceModel::ServicePackageVersion const & get_Version() const { return version_; }

        uint64 get_InstanceId() const { return instanceId_; }
        __declspec(property(get=get_InstanceId)) uint64 InstanceId;

        ServicePackageVersionInstance const & operator = (ServicePackageVersionInstance const & other);
        ServicePackageVersionInstance const & operator = (ServicePackageVersionInstance && other);

        int compare(ServicePackageVersionInstance const & other) const;
        bool operator < (ServicePackageVersionInstance const & other) const;

        bool operator == (ServicePackageVersionInstance const & other) const;
        bool operator != (ServicePackageVersionInstance const & other) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
        
        std::wstring ToString() const;
        static Common::ErrorCode FromString(
            std::wstring const & servicePackageVersionInstanceString, 
            __out ServicePackageVersionInstance & servicePackageVersionInstance);

        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;
        static Common::ErrorCode FromEnvironmentMap(
            Common::EnvironmentMap const & envMap, 
            __out ServicePackageVersionInstance & servicePackageVersionInstance);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {            
            std::string format = "{0}:{1}";
            size_t index = 0;

            traceEvent.AddEventField<ServiceModel::ServicePackageVersion>(format, name + ".servicePackageVersion", index);
            traceEvent.AddEventField<uint64>(format, name + ".instanceId", index);
            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.Write<ServiceModel::ServicePackageVersion>(version_);
            context.Write<uint64>(instanceId_);
        }
        
        const static ServicePackageVersionInstance Invalid;

        FABRIC_FIELDS_02(version_, instanceId_);

    private:
        ServiceModel::ServicePackageVersion version_;
        uint64 instanceId_;

        static Common::GlobalWString Delimiter;
        static Common::GlobalWString EnvVarName_ServicePackageVersionInstance;
    };
}

