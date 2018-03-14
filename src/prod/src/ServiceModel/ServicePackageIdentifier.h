// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ServicePackageIdentifier;
    typedef std::unique_ptr<ServicePackageIdentifier> ServicePackageIdentifierUPtr;

    class ServicePackageIdentifier : public Serialization::FabricSerializable
    {
    public:
        ServicePackageIdentifier();
        ServicePackageIdentifier(
            std::wstring const & applicationIdString,
            std::wstring const & servicePackageName);
        ServicePackageIdentifier(
            ApplicationIdentifier const & applicationId,
            std::wstring const & servicePackageName);
        ServicePackageIdentifier(ServicePackageIdentifier const & other);
        ServicePackageIdentifier(ServicePackageIdentifier && other);

        __declspec(property(get=get_ApplicationId, put=set_ApplicationId)) ApplicationIdentifier const & ApplicationId;
        inline ApplicationIdentifier const & get_ApplicationId() const { return applicationIdentifier_; };
        void set_ApplicationId(ApplicationIdentifier const & value) { applicationIdentifier_ = value; }

        __declspec(property(get=get_ServicePackageName)) std::wstring const & ServicePackageName;
        inline std::wstring const & get_ServicePackageName() const { return servicePackageName_; };

        bool IsEmpty() const;

        ServicePackageIdentifier const & operator = (ServicePackageIdentifier const & other);
        ServicePackageIdentifier const & operator = (ServicePackageIdentifier && other);

        bool operator == (ServicePackageIdentifier const & other) const;
        bool operator != (ServicePackageIdentifier const & other) const;

        int compare(ServicePackageIdentifier const & other) const;
        bool operator < (ServicePackageIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & packageIdString, __out ServicePackageIdentifier & servicePackageId);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {
            std::string format = "{0}";
            size_t index = 0;

            traceEvent.AddEventField<std::wstring>(format, name + ".id", index);

            return format;
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.WriteCopy<std::wstring>(GetServicePackageIdentifierString(applicationIdentifier_, servicePackageName_));
        }

        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;
        static Common::ErrorCode FromEnvironmentMap(Common::EnvironmentMap const & envMap, __out ServicePackageIdentifier & servicePackageId);

        static std::wstring GetServicePackageIdentifierString(std::wstring const & applicationId, std::wstring const & servicePackageName);
        static std::wstring GetServicePackageIdentifierString(ApplicationIdentifier const & applicationId, std::wstring const & servicePackageName);

        FABRIC_FIELDS_02(applicationIdentifier_, servicePackageName_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationIdentifier_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(servicePackageName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        ApplicationIdentifier applicationIdentifier_;
        std::wstring servicePackageName_;

        static Common::GlobalWString Delimiter;
        static Common::GlobalWString EnvVarName_ServicePackageName;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ServicePackageIdentifier);

