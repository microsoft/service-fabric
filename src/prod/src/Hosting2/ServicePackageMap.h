// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{
    class ServicePackage2Map
    {
         DENY_COPY(ServicePackage2Map)

    public:
        ServicePackage2Map();
        ~ServicePackage2Map();

        std::vector<ServicePackage2SPtr> Close();
        Common::ErrorCode Add(ServicePackage2SPtr const & servicePackage);

        Common::ErrorCode Remove(
            std::wstring const & servicePackageName, 
            ServiceModel::ServicePackageActivationContext const & activationContext,
            int64 const instanceId, 
            __out ServicePackage2SPtr & servicePackage);
        
        Common::ErrorCode Find(
            ServicePackageInstanceIdentifier const & servicePackageInstanceId, 
            __out ServicePackage2SPtr & servicePackage) const;

        Common::ErrorCode GetInstancesOfServicePackage(
            std::wstring const & servicePackageName, 
            __out std::vector<ServicePackage2SPtr> & servicePackageInstances) const;

        Common::ErrorCode GetAllServicePackageInstances(__out std::vector<ServicePackage2SPtr> & servicePackageInstances) const;
            
        Common::ErrorCode Find(
            std::wstring const & servicePackageName, 
			std::wstring const & publicActivationId,
            __out ServicePackage2SPtr & servicePackage) const;

		Common::ErrorCode Find(
			std::wstring const & servicePackageName,
			ServiceModel::ServicePackageActivationContext const & activationContext,
			__out ServicePackage2SPtr & servicePackage) const;

        Common::ErrorCode Find(
            std::wstring const & servicePackageName,
            ServiceModel::ServicePackageVersion const & version,
            __out ServicePackage2SPtr & servicePackage) const;

    private:
		Common::ErrorCode Find_CallerHoldsLock(
			std::wstring const & servicePackageName,
			ServiceModel::ServicePackageActivationContext const & activationContext,
			__out ServicePackage2SPtr & servicePackage) const;

		std::wstring GetActivationIdMapKey(
			std::wstring const & servicePackageName,
			std::wstring const & publicActivationId) const;

        mutable Common::RwLock lock_;
        bool closed_;

        typedef std::map<ServiceModel::ServicePackageActivationContext, ServicePackage2SPtr> ServicePackageInstanceMap;
        typedef std::shared_ptr<ServicePackageInstanceMap> ServicePackageInstanceMapSPtr;

        std::map<std::wstring /* ServicePackageName */, ServicePackageInstanceMapSPtr, Common::IsLessCaseInsensitiveComparer<std::wstring>> map_;
		std::map<std::wstring /* <ServicePackagename>_<ServicePackageActivationId> */, ServiceModel::ServicePackageActivationContext> activationIdToContextMap_;
    };
}
