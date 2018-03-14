// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class ServicePackageInstanceContext
    {
    public:
        ServicePackageInstanceContext();
        ServicePackageInstanceContext(
            std::wstring const & servicePackageName,
            ServiceModel::ServicePackageActivationContext activationContext,
			std::wstring const & servicePackagePublicActivationId,
            std::wstring const & applicationName,
            ServiceModel::ApplicationIdentifier const & applicationId,            
            ApplicationEnvironmentContextSPtr const & applicationEnvironment);
        ServicePackageInstanceContext(ServicePackageInstanceContext const & other);
        ServicePackageInstanceContext(ServicePackageInstanceContext && other);

        __declspec(property(get = get_ServicePackageInstanceId)) ServicePackageInstanceIdentifier const & ServicePackageInstanceId;
        ServicePackageInstanceIdentifier const & get_ServicePackageInstanceId() const { return servicePackageInstanceId_; };

        __declspec(property(get=get_ServicePackageName)) std::wstring const & ServicePackageName;
        std::wstring const & get_ServicePackageName() const { return servicePackageInstanceId_.ServicePackageName; }

        __declspec(property(get=get_ApplicationName)) std::wstring const & ApplicationName;
        std::wstring const & get_ApplicationName() const { return applicationName_; };

        __declspec(property(get=get_ApplicationEnvironment)) ApplicationEnvironmentContextSPtr const & ApplicationEnvironment;
        ApplicationEnvironmentContextSPtr const & get_ApplicationEnvironment() const { return applicationEnvironment_; };

        ServicePackageInstanceContext const & operator = (ServicePackageInstanceContext const & other);
        ServicePackageInstanceContext const & operator = (ServicePackageInstanceContext && other);

    private:
        ServicePackageInstanceIdentifier servicePackageInstanceId_;
        ApplicationEnvironmentContextSPtr applicationEnvironment_;
        std::wstring applicationName_;
    };
}
