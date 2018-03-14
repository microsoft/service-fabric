// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ServiceFactoryRegistration
    {
        DENY_COPY(ServiceFactoryRegistration)

    public:
          ServiceFactoryRegistration(
            std::wstring const & originalServiceTypeName,
			  ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ComProxyStatelessServiceFactorySPtr const & statelessServiceFactory);

        ServiceFactoryRegistration(
            std::wstring const & originalServiceTypeName,
			ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
            ComProxyStatefulServiceFactorySPtr const & statefulServiceFactory);
      
        __declspec(property(get = get_ServiceTypeId)) ServiceModel::ServiceTypeIdentifier const & ServiceTypeId;
        inline ServiceModel::ServiceTypeIdentifier const & get_ServiceTypeId() const { return serviceTypeId_; };

        __declspec(property(get=get_OriginalServiceTypeName)) std::wstring const & OriginalServiceTypeName;
        inline std::wstring const & get_OriginalServiceTypeName() const { return originalServiceTypeName_; };
        
        __declspec(property(get=get_StatelessFactory)) ComProxyStatelessServiceFactorySPtr const & StatelessFactory;
        inline ComProxyStatelessServiceFactorySPtr const & get_StatelessFactory() const { return statelessFactory_; };

        __declspec(property(get=get_StatefulFactory)) ComProxyStatefulServiceFactorySPtr const & StatefulFactory;
        inline ComProxyStatefulServiceFactorySPtr const & get_StatefulFactory() const { return statefulFactory_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
		ServiceModel::ServiceTypeIdentifier const serviceTypeId_;
        std::wstring const originalServiceTypeName_;
        ComProxyStatelessServiceFactorySPtr const statelessFactory_;
        ComProxyStatefulServiceFactorySPtr const statefulFactory_;
    };
}
