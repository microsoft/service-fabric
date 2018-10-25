// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{ 
    class IRuntimeSharingHelper;
    typedef std::unique_ptr<IRuntimeSharingHelper> IRuntimeSharingHelperUPtr;

    class ApplicationHostActivationContext;
    typedef std::shared_ptr<ApplicationHostActivationContext> ApplicationHostActivationContextSPtr;

    class IApplicationHost;
    class ApplicationHost;
    typedef std::shared_ptr<ApplicationHost> ApplicationHostSPtr;

    class InProcessApplicationHost;
    typedef std::shared_ptr<InProcessApplicationHost> InProcessApplicationHostSPtr;

    class ComFabricRuntime;
    class ComGuestServiceCodePackageActivationManager;
    
    class FabricRuntimeContext;
    typedef std::unique_ptr<FabricRuntimeContext> FabricRuntimeContextUPtr;

    class FabricRuntimeImpl;
    typedef std::shared_ptr<FabricRuntimeImpl> FabricRuntimeImplSPtr;

    class FabricNodeContextResult;
    typedef std::shared_ptr<FabricNodeContextResult> FabricNodeContextResultImplSPtr;
    
    class ServiceFactoryManager;
    typedef std::unique_ptr<ServiceFactoryManager> ServiceFactoryManagerUPtr;

    class ServiceFactoryRegistrationTable;
    typedef std::unique_ptr<ServiceFactoryRegistrationTable> ServiceFactoryRegistrationTableUPtr;

    class ServiceFactoryRegistration;
    typedef std::shared_ptr<ServiceFactoryRegistration> ServiceFactoryRegistrationSPtr;

    class ComProxyStatelessServiceFactory;
    typedef std::shared_ptr<ComProxyStatelessServiceFactory> ComProxyStatelessServiceFactorySPtr;

    class ComProxyStatefulServiceFactory;
    typedef std::shared_ptr<ComProxyStatefulServiceFactory> ComProxyStatefulServiceFactorySPtr;

    class LeaseMonitor;
    typedef std::unique_ptr<LeaseMonitor> LeaseMonitorUPtr;

    class RuntimeSharingHelper;
    typedef std::unique_ptr<RuntimeSharingHelper> RuntimeSharingHelperUPtr;

    class CodePackageActivationContext;
    typedef std::shared_ptr<CodePackageActivationContext> CodePackageActivationContextSPtr;

#if !defined (PLATFORM_UNIX)
    class ComFabricBackupRestoreAgent;

    class FabricBackupRestoreAgentImpl;
    typedef std::shared_ptr<FabricBackupRestoreAgentImpl> FabricBackupRestoreAgentImplSPtr;

    class FabricGetBackupSchedulePolicyResult;
    typedef std::shared_ptr<FabricGetBackupSchedulePolicyResult> FabricGetBackupSchedulePolicyResultImplSPtr;

    class FabricGetRestorePointDetailsResult;
    typedef std::shared_ptr<FabricGetRestorePointDetailsResult> FabricGetRestorePointDetailsResultImplSPtr;
#endif
}
