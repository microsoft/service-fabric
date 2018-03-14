// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServicePackageVersion : public Serialization::FabricSerializable
    {
    public:
        ServicePackageVersion();
        ServicePackageVersion(ApplicationVersion const & appVersion, RolloutVersion const & value);
        ServicePackageVersion(ServicePackageVersion const & other);
        ServicePackageVersion(ServicePackageVersion && other);
        virtual ~ServicePackageVersion();

        static const ServicePackageVersion Zero;

        __declspec(property(get=get_applicationVersion)) ApplicationVersion const & ApplicationVersionValue;
        ApplicationVersion const & get_applicationVersion() const { return appVersion_; }

        __declspec(property(get=get_Value)) RolloutVersion const & RolloutVersionValue;
        RolloutVersion const & get_Value() const { return value_; }

        ServicePackageVersion const & operator = (ServicePackageVersion const & other);
        ServicePackageVersion const & operator = (ServicePackageVersion && other);

        bool operator == (ServicePackageVersion const & other) const;
        bool operator != (ServicePackageVersion const & other) const;

        int compare(ServicePackageVersion const & other) const;
        bool operator < (ServicePackageVersion const & other) const;

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & servicePackageVersionString, __out ServicePackageVersion & servicePackageVersion);

        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;
        static Common::ErrorCode FromEnvironmentMap(Common::EnvironmentMap const & envMap, __out ServicePackageVersion & servicePackageVersion);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
        void FillEventData(Common::TraceEventContext & context) const;

        const static ServicePackageVersion Invalid;

        FABRIC_FIELDS_02(appVersion_, value_);

    private:
        ApplicationVersion appVersion_;
        RolloutVersion value_;

        static Common::GlobalWString Delimiter;
        static Common::GlobalWString EnvVarName_ServicePackageVersion;
    };
}
