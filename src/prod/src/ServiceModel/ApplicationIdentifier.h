// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class ApplicationIdentifier : public Serialization::FabricSerializable
    {
    public:
        static Common::Global<ApplicationIdentifier> FabricSystemAppId;

        ApplicationIdentifier();
        ApplicationIdentifier(std::wstring const & applicationTypeName, ULONG applicationNumber);
        ApplicationIdentifier(ApplicationIdentifier const & other);
        ApplicationIdentifier(ApplicationIdentifier && other);

        __declspec(property(get=get_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
        inline std::wstring const & get_ApplicationTypeName() const { return applicationTypeName_; };

        __declspec(property(get=get_ApplicationNumber)) ULONG ApplicationNumber;
        inline ULONG get_ApplicationNumber() const { return applicationNumber_; };

        bool IsEmpty() const;
        bool IsAdhoc() const;
        bool IsSystem() const;

        ApplicationIdentifier const & operator = (ApplicationIdentifier const & other);
        ApplicationIdentifier const & operator = (ApplicationIdentifier && other);

        bool operator == (ApplicationIdentifier const & other) const;
        bool operator != (ApplicationIdentifier const & other) const;

        int compare(ApplicationIdentifier const & other) const;
        bool operator < (ApplicationIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        std::wstring ToString() const;
        static Common::ErrorCode FromString(std::wstring const & applicationIdString, __out ApplicationIdentifier & applicationId);

        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;
        static Common::ErrorCode FromEnvironmentMap(Common::EnvironmentMap const& envMap, __out ApplicationIdentifier & applicationId);

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

        void FillEventData(Common::TraceEventContext & context) const;

        FABRIC_FIELDS_02(applicationTypeName_, applicationNumber_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(applicationTypeName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        std::wstring applicationTypeName_;
        ULONG applicationNumber_;
        static Common::GlobalWString Delimiter;
        static Common::GlobalWString EnvVarName_ApplicationId;
    };
}

DEFINE_USER_ARRAY_UTILITY(ServiceModel::ApplicationIdentifier);
