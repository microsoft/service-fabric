// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    namespace ServicePackageActivationMode
    {
        enum Enum : ULONG
        {
            SharedProcess = 0,
            ExclusiveProcess = 1,
            FirstValidEnum = SharedProcess,
            LastValidEnum = ExclusiveProcess
        };

        void WriteToTextWriter(__in Common::TextWriter & w, Enum const & e);

        ::FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE ToPublicApi(__in Enum);
        Common::ErrorCode FromPublicApi(__in ::FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE, __out Enum &);

        DECLARE_ENUM_STRUCTURED_TRACE(ServicePackageActivationMode)

        BEGIN_DECLARE_ENUM_JSON_SERIALIZER(Enum)
            ADD_ENUM_VALUE(SharedProcess)
            ADD_ENUM_VALUE(ExclusiveProcess)
        END_DECLARE_ENUM_SERIALIZER()
    };

    class ServicePackageActivationContext : public Serialization::FabricSerializable
    {
    public:
        ServicePackageActivationContext();
        explicit ServicePackageActivationContext(Common::Guid const & activationId);

        ServicePackageActivationContext(ServicePackageActivationContext const & other);
        ServicePackageActivationContext(ServicePackageActivationContext && other);

        __declspec(property(get = get_IsExclusive)) bool IsExclusive;
        inline bool get_IsExclusive() const { return (activationMode_ == ServicePackageActivationMode::ExclusiveProcess); }

        __declspec(property(get = get_ActivationMode)) ServicePackageActivationMode::Enum ActivationMode;
        inline ServicePackageActivationMode::Enum get_ActivationMode() const { return activationMode_; }
		
        __declspec(property(get = get_ActivationGuid)) Common::Guid const & ActivationGuid;
        inline Common::Guid const & get_ActivationGuid() const
        {
            return activationGuid_;
        }
        
        ServicePackageActivationContext const & operator = (ServicePackageActivationContext const & other);
        ServicePackageActivationContext const & operator = (ServicePackageActivationContext && other);

        bool operator == (ServicePackageActivationContext const & other) const;
        bool operator != (ServicePackageActivationContext const & other) const;

        int compare(ServicePackageActivationContext const & other) const;
        bool operator < (ServicePackageActivationContext const & other) const;

        inline std::wstring ToString() const
        {
            if (this->IsExclusive)
            {
                return activationGuid_.ToString();
            }

            return std::wstring();
        }
        
        static Common::ErrorCode FromString(
            std::wstring const & isolationContextStr, 
            __out ServicePackageActivationContext & isolationContext);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;
        static Common::ErrorCode FromEnvironmentMap(
            Common::EnvironmentMap const & envMap,
            __out ServicePackageActivationContext & isolationContext);

        FABRIC_FIELDS_02(activationGuid_, activationMode_);

    private:
        Common::Guid activationGuid_;
        ServicePackageActivationMode::Enum activationMode_;

        static Common::GlobalWString EnvVarName_ActivationGuid;
    };
}
