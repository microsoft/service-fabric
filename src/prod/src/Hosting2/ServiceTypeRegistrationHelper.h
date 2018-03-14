// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Hosting2
{
    inline ServiceTypeRegistrationSPtr CreateServiceTypeRegistration(
        ServiceModel::ServiceTypeIdentifier const & serviceTypeId,
        ServiceModel::ServicePackageActivationContext const & activationContext,
        std::wstring const & servicePackagePublicActivationId,
        FabricRuntimeContext const & fabricRuntimeContext)
    {
        return std::make_shared<ServiceTypeRegistration>(
            serviceTypeId,
            fabricRuntimeContext.CodeContext.servicePackageVersionInstance,
            activationContext,
			servicePackagePublicActivationId,
            fabricRuntimeContext.HostContext.HostId,
            fabricRuntimeContext.HostContext.ProcessId,
            fabricRuntimeContext.RuntimeId,
            fabricRuntimeContext.CodeContext.CodePackageInstanceId.CodePackageName,
            fabricRuntimeContext.CodeContext.ServicePackageInstanceSeqNum);
    }
}
