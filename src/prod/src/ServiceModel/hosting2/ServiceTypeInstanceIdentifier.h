// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServiceTypeInstanceIdentifier : public Serialization::FabricSerializable
    {
    public:
        ServiceTypeInstanceIdentifier();

        ServiceTypeInstanceIdentifier(
            ServiceModel::ServiceTypeIdentifier const & typeIdentifier,
            ServiceModel::ServicePackageActivationContext const & activationContext,
			std::wstring const & servicePackageActivationId);

        ServiceTypeInstanceIdentifier(
            ServicePackageInstanceIdentifier const & packageInstanceIdentifier,
            std::wstring const & serviceTypeName);
        
        ServiceTypeInstanceIdentifier(ServiceTypeInstanceIdentifier const & other);
        ServiceTypeInstanceIdentifier(ServiceTypeInstanceIdentifier && other);

        __declspec(property(get = get_ServicePackageInstanceId)) ServicePackageInstanceIdentifier const & ServicePackageInstanceId;
        inline ServicePackageInstanceIdentifier const & get_ServicePackageInstanceId() const { return packageInstanceIdentifier_; }

        __declspec(property(get = get_ApplicationId, put = set_ApplicationId)) ServiceModel::ApplicationIdentifier const & ApplicationId;
        inline ServiceModel::ApplicationIdentifier const & get_ApplicationId() const 
        { 
            return packageInstanceIdentifier_.ServicePackageId.ApplicationId; 
        }

        __declspec(property(get = get_ServiceTypeName)) std::wstring const & ServiceTypeName;
        inline std::wstring const & get_ServiceTypeName() const { return serviceTypeName_; };

        __declspec(property(get = get_ActivationContext))  ServiceModel::ServicePackageActivationContext const & ActivationContext;
        inline  ServiceModel::ServicePackageActivationContext const & get_ActivationContext() const
        { 
            return packageInstanceIdentifier_.ActivationContext; 
        }

        __declspec(property(get = get_PublicActivationId))  std::wstring const & PublicActivationId;
        inline  std::wstring const & get_PublicActivationId() const
        {
            return packageInstanceIdentifier_.PublicActivationId;
        }

        __declspec(property(get = get_ServiceTypeId))  ServiceModel::ServiceTypeIdentifier const & ServiceTypeId;
        inline  ServiceModel::ServiceTypeIdentifier const & get_ServiceTypeId() const
        {
            if (serviceTypeId_.ServicePackageId.IsEmpty())
            {
                serviceTypeId_ = ServiceModel::ServiceTypeIdentifier(packageInstanceIdentifier_.ServicePackageId, serviceTypeName_);
            }

            return serviceTypeId_;
        }

        ServiceTypeInstanceIdentifier const & operator = (ServiceTypeInstanceIdentifier const & other);
        ServiceTypeInstanceIdentifier const & operator = (ServiceTypeInstanceIdentifier && other);

        bool operator == (ServiceTypeInstanceIdentifier const & other) const;
        bool operator != (ServiceTypeInstanceIdentifier const & other) const;

        int compare(ServiceTypeInstanceIdentifier const & other) const;
        bool operator < (ServiceTypeInstanceIdentifier const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        std::wstring ToString() const;
        
        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
        {
            traceEvent.AddField<std::wstring>(name + ".identifier");
            return "{0}";
        }

        void FillEventData(Common::TraceEventContext & context) const
        {
            context.WriteCopy<std::wstring>(ToString());
        }

        FABRIC_FIELDS_02(packageInstanceIdentifier_, serviceTypeName_);

        BEGIN_DYNAMIC_SIZE_ESTIMATION()
            DYNAMIC_SIZE_ESTIMATION_MEMBER(packageInstanceIdentifier_)
            DYNAMIC_SIZE_ESTIMATION_MEMBER(serviceTypeName_)
        END_DYNAMIC_SIZE_ESTIMATION()

    private:
        ServicePackageInstanceIdentifier packageInstanceIdentifier_;
        std::wstring serviceTypeName_;
        mutable ServiceModel::ServiceTypeIdentifier serviceTypeId_;

        static Common::GlobalWString Delimiter;
    };
}

DEFINE_USER_ARRAY_UTILITY(Hosting2::ServiceTypeInstanceIdentifier);
