// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServicePackageInstanceIdentifier;
    typedef std::unique_ptr<ServicePackageInstanceIdentifier> ServicePackageInstanceIdentifierUPtr;

    class ServicePackageInstanceIdentifier : public Serialization::FabricSerializable
    {
    public:
        ServicePackageInstanceIdentifier();

        ServicePackageInstanceIdentifier(
            ServiceModel::ServicePackageIdentifier const & servicePackageId,
            ServiceModel::ServicePackageActivationContext const & activationContext,
			std::wstring const & servicePackageActivationId);

        ServicePackageInstanceIdentifier(ServicePackageInstanceIdentifier const & other);
        ServicePackageInstanceIdentifier(ServicePackageInstanceIdentifier && other);

        __declspec(property(get = get_ServicePackageId)) ServiceModel::ServicePackageIdentifier const & ServicePackageId;
        inline ServiceModel::ServicePackageIdentifier const & get_ServicePackageId() const { return servicePackageId_; }

        __declspec(property(get = get_ActivationContext)) ServiceModel::ServicePackageActivationContext const & ActivationContext;
        inline ServiceModel::ServicePackageActivationContext const & get_ActivationContext() const { return activationContext_; }

		__declspec(property(get = get_PublicActivatonId)) std::wstring const & PublicActivationId;
		inline std::wstring const & get_PublicActivatonId() const { return servicePackageActivationId_; }

        __declspec(property(get = get_ServicePackageName)) std::wstring const & ServicePackageName;
        inline std::wstring const & get_ServicePackageName() const { return servicePackageId_.ServicePackageName; }

        __declspec(property(get = get_ApplicationId)) ServiceModel::ApplicationIdentifier const & ApplicationId;
        inline ServiceModel::ApplicationIdentifier const & get_ApplicationId() const { return servicePackageId_.ApplicationId; };

        ServicePackageInstanceIdentifier const & operator = (ServicePackageInstanceIdentifier const & other);
        ServicePackageInstanceIdentifier const & operator = (ServicePackageInstanceIdentifier && other);

        bool operator == (ServicePackageInstanceIdentifier const & other) const;
        bool operator != (ServicePackageInstanceIdentifier const & other) const;

        int compare(ServicePackageInstanceIdentifier const & other) const;
        bool operator < (ServicePackageInstanceIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        
        std::wstring ToString() const;

        static Common::ErrorCode FromString(
            std::wstring const & packageInstanceIdString, 
            __out ServicePackageInstanceIdentifier & servicePackageInstanceId);
        
        bool IsActivationOf(ServiceModel::ServicePackageIdentifier servicePackageId) const;

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {
            traceEvent.AddField<std::wstring>(name + ".servicePackageInstanceIdentifier");
            return "[{0}:{1}]";
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.WriteCopy<std::wstring>(this->ToString());
        }

        void ToEnvironmentMap(Common::EnvironmentMap & envMap) const;
        static Common::ErrorCode FromEnvironmentMap(
            Common::EnvironmentMap const & envMap, 
            __out ServicePackageInstanceIdentifier & servicePackageInstanceId);

        FABRIC_FIELDS_03(servicePackageId_, activationContext_, servicePackageActivationId_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(servicePackageId_)
			DYNAMIC_SIZE_ESTIMATION_MEMBER(servicePackageActivationId_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
		static void Validate(
			ServiceModel::ServicePackageActivationContext const & activationContext,
			std::wstring const & servicePackageActivationId);

        ServiceModel::ServicePackageIdentifier servicePackageId_;
        ServiceModel::ServicePackageActivationContext activationContext_;
		std::wstring servicePackageActivationId_;

        static Common::GlobalWString Delimiter;
		static Common::GlobalWString EnvVarName_ServicePackageActivationId;
    };

    
}
