// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceSource("ServicePackageActivationContext");

namespace ServiceModel
{
    namespace ServicePackageActivationMode
    {
        void WriteToTextWriter(__in TextWriter & w, Enum const & e)
        {
            switch (e)
            {
            case ServicePackageActivationMode::SharedProcess: w << "SharedProcess"; return;
            case ServicePackageActivationMode::ExclusiveProcess: w << "ExclusiveProcess"; return;
            default: w << "ServicePackageActivationMode(" << static_cast<ULONG>(e) << ')'; return;
            }
        }

        FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE ToPublicApi(__in Enum internalLevel)
        {
            switch (internalLevel)
            {
            case SharedProcess:
                return FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS;

            case ExclusiveProcess:
                return FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_EXCLUSIVE_PROCESS;

            default:
                return FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS;
            }
        }

        ErrorCode FromPublicApi(__in FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE publicLevel, __out Enum & internalLevel)
        {
            switch (publicLevel)
            {
            case FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_SHARED_PROCESS:
                internalLevel = SharedProcess;
                return ErrorCodeValue::Success;

            case FABRIC_SERVICE_PACKAGE_ACTIVATION_MODE_EXCLUSIVE_PROCESS:
                internalLevel = ExclusiveProcess;
                return ErrorCodeValue::Success;

            default:
                return ErrorCodeValue::InvalidArgument;
            }
        }

        ENUM_STRUCTURED_TRACE(ServicePackageActivationMode, FirstValidEnum, LastValidEnum)
    }

    GlobalWString ServicePackageActivationContext::EnvVarName_ActivationGuid = make_global<wstring>(L"Fabric_ServicePackageActivationGuid");

    ServicePackageActivationContext::ServicePackageActivationContext()
        : activationGuid_()
        , activationMode_(ServicePackageActivationMode::SharedProcess)
    {
    }

    ServicePackageActivationContext::ServicePackageActivationContext(Guid const & partitionId)
        : activationGuid_(partitionId)
        , activationMode_(ServicePackageActivationMode::ExclusiveProcess)
    {
        ASSERT_IF(partitionId == Guid::Empty(), "ActivationId supplied to ServicePackageActivationContext is empty.");
    }

    ServicePackageActivationContext::ServicePackageActivationContext(ServicePackageActivationContext const & other)
        : activationGuid_(other.activationGuid_)
        , activationMode_(other.activationMode_)
    {
    }

    ServicePackageActivationContext::ServicePackageActivationContext(ServicePackageActivationContext && other)
        : activationGuid_(std::move(other.activationGuid_))
        , activationMode_(std::move(other.activationMode_))
    {
    }

    ServicePackageActivationContext const & ServicePackageActivationContext::operator = (ServicePackageActivationContext const & other)
    {
        if (this != &other)
        {
            this->activationGuid_ = other.activationGuid_;
            this->activationMode_ = other.activationMode_;
        }

        return *this;
    }

    ServicePackageActivationContext const & ServicePackageActivationContext::operator = (ServicePackageActivationContext && other)
    {
        if (this != &other)
        {
            this->activationGuid_ = std::move(other.activationGuid_);
            this->activationMode_ = std::move(other.activationMode_);
        }

        return *this;
    }

    bool ServicePackageActivationContext::operator == (ServicePackageActivationContext const & other) const
    {
        return (activationGuid_ == other.activationGuid_);
    }

    bool ServicePackageActivationContext::operator != (ServicePackageActivationContext const & other) const
    {
        return !(*this == other);
    }

    int ServicePackageActivationContext::compare(ServicePackageActivationContext const & other) const
    {
        return StringUtility::Compare(activationGuid_.ToString(), other.activationGuid_.ToString());
    }

    bool ServicePackageActivationContext::operator < (ServicePackageActivationContext const & other) const
    {
        return (this->compare(other) < 0);
    }

    ErrorCode ServicePackageActivationContext::FromString(
        wstring const & activationContextStr, 
        __out ServicePackageActivationContext & activationContext)
    {
        if (activationContextStr.empty())
        {
            activationContext = ServicePackageActivationContext();
            return ErrorCode(ErrorCodeValue::Success);
        }

        Guid activationGuid;
        if (!Guid::TryParse(activationContextStr, activationGuid))
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument);
        }

        activationContext = ServicePackageActivationContext(activationGuid);
        return ErrorCode(ErrorCodeValue::Success);
    }

    void ServicePackageActivationContext::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w.Write(this->ToString());
    }

    void ServicePackageActivationContext::ToEnvironmentMap(Common::EnvironmentMap & envMap) const
    {
        if (this->IsExclusive)
        {
            envMap[ServicePackageActivationContext::EnvVarName_ActivationGuid] = activationGuid_.ToString();
        }
    }
    
    ErrorCode ServicePackageActivationContext::FromEnvironmentMap(
        EnvironmentMap const & envMap, 
        __out ServicePackageActivationContext & activationContext)
    {
        auto activationIdIterator = envMap.find(ServicePackageActivationContext::EnvVarName_ActivationGuid);
        if (activationIdIterator != envMap.end())
        {
			Guid activationGuid;
			if (!Guid::TryParse(activationIdIterator->second, activationGuid))
			{
				Trace.WriteError(
					TraceSource, 
					"FromEnvironmentMap: Invalid Guid string value", 
					activationIdIterator->second);

				return ErrorCode(ErrorCodeValue::InvalidArgument);
			}

            activationContext = ServicePackageActivationContext(activationGuid);
        }
        else
        {
            activationContext = ServicePackageActivationContext();
        }
        
        return ErrorCode(ErrorCodeValue::Success);
    }
}
