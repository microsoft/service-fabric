// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{

#define DECLARE_CREATE_COM_CLIENT( IFabricClient, Func ) \
    ComPointer< IFabricClient > Create##Func(__in FabricTestFederation &);

#define DEFINE_CREATE_COM_CLIENT( IFabricClient, Func ) \
    ComPointer< IFabricClient > TestFabricClientUpgrade::Create##Func(__in FabricTestFederation & testFederation) \
    { \
        ComPointer< IFabricClient > result; \
        HRESULT hr = TestFabricClient::FabricCreateComFabricClient(testFederation)->QueryInterface( \
            IID_##IFabricClient, \
            (void **)result.InitializationAddress()); \
        TestSession::FailTestIfNot(SUCCEEDED(hr), "ComFabricClient does not support IFabricClient"); \
        return result; \
    } \

    class FabricTestSession;

    class TestFabricClientUpgrade
    {
        DENY_COPY(TestFabricClientUpgrade);

        friend class TestFabricClientScenarios;

    public:
        TestFabricClientUpgrade(std::shared_ptr<TestFabricClient> testClientSPtr)
        {
            testClientSPtr_ = testClientSPtr;
        };
        virtual ~TestFabricClientUpgrade() {}

        __declspec (property(get=getTestClientSptr)) std::shared_ptr<TestFabricClient> & TestFabricClientSPtr;

        bool ParallelProvisionApplicationType(Common::StringCollection const & params);
        bool ProvisionApplicationType(Common::StringCollection const & params);
        bool UnprovisionApplicationType(Common::StringCollection const & params);
        bool CreateApplication(Common::StringCollection const & params);
        bool UpdateApplication(Common::StringCollection const & params);
        bool DeleteApplication(Common::StringCollection const & params);
        bool UpgradeApplication(Common::StringCollection const & params);
        bool UpdateApplicationUpgrade(Common::StringCollection const & params);
        bool VerifyApplicationUpgradeDescription(Common::StringCollection const & params);
        bool RollbackApplicationUpgrade(Common::StringCollection const & params);
        bool SetRollbackApplication(Common::StringCollection const & params);
        bool UpgradeAppStatus(Common::StringCollection const & params);
        bool MoveNextApplicationUpgradeDomain(Common::StringCollection const & params);

        bool CreateComposeDeployment(Common::StringCollection const & params);
        bool DeleteComposeDeployment(Common::StringCollection const & params);
        bool UpgradeComposeDeployment(Common::StringCollection const & params);
        bool RollbackComposeDeployment(Common::StringCollection const & params);

        bool ProvisionFabric(Common::StringCollection const & params);
        bool UnprovisionFabric(Common::StringCollection const & params);        
        bool UpgradeFabric(Common::StringCollection const & params);
        bool UpdateFabricUpgrade(Common::StringCollection const & params);
        bool VerifyFabricUpgradeDescription(Common::StringCollection const & params);
        bool RollbackFabricUpgrade(Common::StringCollection const & params);
        bool SetRollbackFabric(Common::StringCollection const & params);
        bool UpgradeFabricStatus(Common::StringCollection const & params);
        bool MoveNextFabricUpgradeDomain(Common::StringCollection const & params);

        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient, FabricApplicationClient )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient2, FabricApplicationClient2 )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient3, FabricApplicationClient3 )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient4, FabricApplicationClient4 )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient5, FabricApplicationClient5 )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient6, FabricApplicationClient6 )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient7, FabricApplicationClient7 )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient8, FabricApplicationClient8 )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient9, FabricApplicationClient9 )
        DECLARE_CREATE_COM_CLIENT( IFabricApplicationManagementClient10, FabricApplicationClient10 )

        DECLARE_CREATE_COM_CLIENT( IFabricClusterManagementClient, FabricClusterClient )
        DECLARE_CREATE_COM_CLIENT( IFabricClusterManagementClient2, FabricClusterClient2 )
        DECLARE_CREATE_COM_CLIENT( IFabricClusterManagementClient3, FabricClusterClient3 )
        DECLARE_CREATE_COM_CLIENT( IFabricClusterManagementClient4, FabricClusterClient4 )

        DECLARE_CREATE_COM_CLIENT( IInternalFabricApplicationManagementClient, InternalFabricApplicationClient )
        DECLARE_CREATE_COM_CLIENT( IInternalFabricApplicationManagementClient2, InternalFabricApplicationClient2 )

    private:
        class ApplicationUpgradeContext;
        typedef std::shared_ptr<ApplicationUpgradeContext> ApplicationUpgradeContextSPtr;

        class FabricUpgradeContext;
        typedef std::shared_ptr<FabricUpgradeContext> FabricUpgradeContextSPtr;

        void ProvisionApplicationTypeImpl(
            std::wstring const& applicationBuildPath,
            bool isAsync,
            Management::ClusterManager::ApplicationPackageCleanupPolicy::Enum applicationPackageCleanupPolicy,
            HRESULT expectedError);

        void ProvisionApplicationTypeImpl(
            std::wstring const& downloadPath,
            std::wstring const& appTypeName,
            std::wstring const& appTypeVersion,
            bool isAsync,
            HRESULT expectedError);

        void UnprovisionApplicationTypeImpl(
            std::wstring const& applicationTypeName,
            std::wstring const& applicationTypeVersion,
            bool isAsync,
            HRESULT expectedError);

        void CreateApplicationImpl(
            FABRIC_APPLICATION_DESCRIPTION const& applicationeDescription,
            HRESULT expectedError);
        void UpdateApplicationImpl(
            FABRIC_APPLICATION_UPDATE_DESCRIPTION const& applicationUpdateDescription,
            HRESULT expectedError);
        void DeleteApplicationImpl(Common::NamingUri const& name, bool const isForce, HRESULT expectedError);
        void CreateComposeDeploymentImpl(FABRIC_COMPOSE_DEPLOYMENT_DESCRIPTION const &, HRESULT expectedErrors);
        void DeleteComposeDeploymentImpl(std::wstring const & name, HRESULT expectedErrors);
        void UpgradeComposeDeploymentImpl(FABRIC_COMPOSE_DEPLOYMENT_UPGRADE_DESCRIPTION &, Common::TimeSpan const, std::vector<HRESULT>);
        void RollbackComposeDeploymentImpl(std::wstring const & name, HRESULT expectedErrors);

        void UpgradeApplicationImpl(FABRIC_APPLICATION_UPGRADE_DESCRIPTION & upgradeDescription, Common::TimeSpan const timeout, std::vector<HRESULT> expectedErrors);
        void UpdateApplicationUpgradeImpl(FABRIC_APPLICATION_UPGRADE_UPDATE_DESCRIPTION &, HRESULT expectedError);
        void RollbackApplicationUpgradeImpl(Common::NamingUri const&, HRESULT expectedError);
        void UpgradeFabricImpl(FABRIC_UPGRADE_DESCRIPTION & upgradeFabricDescription, Common::TimeSpan const timeout, HRESULT expectedError);
        void UpdateFabricUpgradeImpl(FABRIC_UPGRADE_UPDATE_DESCRIPTION &, HRESULT expectedError);
        void RollbackFabricUpgradeImpl(HRESULT expectedError);

        void UpgradeAppStatusImpl(std::wstring const & appName, HRESULT expectedError, __out Common::ComPointer<IFabricApplicationUpgradeProgressResult3> &);
        HRESULT UpgradeAppStatusImpl(std::wstring const & appName, std::vector<HRESULT> expectedErrors, __out Common::ComPointer<IFabricApplicationUpgradeProgressResult3> &);
        void UpgradeFabricStatusImpl(HRESULT expectedError, HRESULT retryableError, __out Common::ComPointer<IFabricUpgradeProgressResult3> &);

        void MoveNextApplicationUpgradeDomainImpl(std::wstring const & appName, HRESULT expectedError);        
        void MoveNextApplicationUpgradeDomainImpl2(std::wstring const & appName, HRESULT expectedError);        
        void MoveNextApplicationUpgradeDomainImpl2(FABRIC_URI appName, LPCWSTR ud, HRESULT expectedError);        

        void MoveNextFabricUpgradeDomainImpl(HRESULT expectedError);        
        void MoveNextFabricUpgradeDomainImpl2(HRESULT expectedError);        
        void MoveNextFabricUpgradeDomainImpl2(LPCWSTR ud, HRESULT expectedError);        

        bool PrintUpgradeProgress(
            bool printDetails,
            Common::ComPointer<IFabricApplicationUpgradeProgressResult3> const& upgradeProgress);
        bool PrintUpgradeChanges(
            Common::ComPointer<IFabricApplicationUpgradeProgressResult3> const& previousProgress,
            Common::ComPointer<IFabricApplicationUpgradeProgressResult3> const& upgradeProgress);
        void CheckUpgradeAppComplete(
            std::wstring const& appName, 
            std::wstring const& appTypeName, 
            std::wstring const& targetVersion,
            std::wstring const& previousAppBuilderName,
            ApplicationUpgradeContextSPtr const &);

        bool PrintUpgradeProgress(
            bool printDetails,
            Common::ComPointer<IFabricUpgradeProgressResult3> const& upgradeProgress);
        bool PrintUpgradeChanges(
            Common::ComPointer<IFabricUpgradeProgressResult3> const& previousProgress,
            Common::ComPointer<IFabricUpgradeProgressResult3> const& upgradeProgress);
        void CheckUpgradeFabricComplete(
            std::wstring const& codeVersion,            
            std::wstring const& configVersion,
            FabricUpgradeContextSPtr const &);

        void PrintUpgradeDomainProgress(FABRIC_UPGRADE_DOMAIN_PROGRESS const* upgradeDomainProgress);

        void VerifyRollingUpgradePolicyDescription(
            __in TestCommon::CommandLineParser &,
            FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION const &);

        void GetRollingUpgradePolicyDescriptionForUpgrade(
            __in TestCommon::CommandLineParser &,
            __inout FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION &);

        void GetRollingUpgradePolicyDescriptionForUpdate(
            __in TestCommon::CommandLineParser &,
            __inout DWORD & updateFlags,
            __inout FABRIC_ROLLING_UPGRADE_POLICY_DESCRIPTION &);

        static Management::ClusterManager::UpgradeFailureReason::Enum ParseUpgradeFailureReason(__in CommandLineParser &);
        static inline std::wstring GetApplicationNameFromDeploymentName(__in std::wstring const & deploymentName)
        {
            return NamingUri(deploymentName).ToString();
        }

        inline std::shared_ptr<TestFabricClient> & getTestClientSptr()
        {
            return testClientSPtr_;
        }

        std::shared_ptr<TestFabricClient> testClientSPtr_;
    };
}
