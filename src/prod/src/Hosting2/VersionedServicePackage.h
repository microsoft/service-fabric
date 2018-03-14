// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class VersionedServicePackage :
        public Common::ComponentRoot,
        public Common::StateMachine,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        DENY_COPY(VersionedServicePackage)

        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteNoise;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteWarning;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteError;
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteTrace;

        STATEMACHINE_STATES_08(Created, Opening, Opened, Switching, Analyzing, Closing, Closed, Failed);
        STATEMACHINE_TRANSITION(Opening, Created);
        STATEMACHINE_TRANSITION(Opened, Opening|Switching|Analyzing);
        STATEMACHINE_TRANSITION(Switching, Opened);
        STATEMACHINE_TRANSITION(Analyzing, Opened);
        STATEMACHINE_TRANSITION(Failed, Opening|Switching|Closing);
        STATEMACHINE_TRANSITION(Closing, Opened);
        STATEMACHINE_TRANSITION(Closed, Closing);
        STATEMACHINE_ABORTED_TRANSITION(Created|Opened|Failed);
        STATEMACHINE_TERMINAL_STATES(Aborted|Closed);

    public:
        VersionedServicePackage(
            ServicePackage2Holder const & servicePackageHolder,            
            ServiceModel::ServicePackageVersionInstance const & versionInstance,
            ServiceModel::ServicePackageDescription const & packageDescription,
            std::vector<ServiceTypeInstanceIdentifier> const & serviceTypeInstanceIds);
        virtual ~VersionedServicePackage();

        // Opens the service package with the version specified in the ctor
        // if Open is attempted and it is successful, the package transitions to Opened state
        // if Open is attempted and it is unsuccessful, the package transitions to Failed state
        Common::AsyncOperationSPtr BeginOpen(            
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndOpen(
            Common::AsyncOperationSPtr const & operation);

        // Switches to the specified version from the currently running version
        // if Switch is attempted and it is successful, the package transitions to Opened state
        // if Switch is attempted and it is unsuccessful, the package transitions to Failed state
        Common::AsyncOperationSPtr BeginSwitch(
            ServiceModel::ServicePackageVersionInstance const & toVersionInstance,
            ServiceModel::ServicePackageDescription toPackageDescription,            
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndSwitch(
            Common::AsyncOperationSPtr const & operation);

        // Closes the opened package. 
        // if Close is attempted and it is successful, the package transitions to Closed state
        // if Close is attempted and it is not successful, the package is Aborted
        Common::AsyncOperationSPtr BeginClose(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        void EndClose(
            Common::AsyncOperationSPtr const & operation);

        Common::ErrorCode AnalyzeApplicationUpgrade(            
            ServiceModel::ServicePackageVersionInstance const & toVersionInstance,
            ServiceModel::ServicePackageDescription const & toPackageDescription,
            __out CaseInsensitiveStringSet & affectedRuntimeIds);
        
        ULONG GetFailureCount() const;        

        std::vector<CodePackageSPtr> GetCodePackages(std::wstring const & filterCodePackageName = L"");

        void OnServiceTypeRegistrationNotFound(
            uint64 const registrationTableVersion,
            ServiceModel::VersionedServiceTypeIdentifier const & versionedServiceTypeId,
            ServicePackageInstanceIdentifier const & servicePackageInstanceId);

        Common::ErrorCode IsCodePackageLockFilePresent(__out bool & isCodePackageLockFilePresent);

        Common::AsyncOperationSPtr BeginRestartCodePackageInstance(
            std::wstring const & codePackageName,
            int64 instanceId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EndRestartCodePackageInstance(
            Common::AsyncOperationSPtr const & operation);

        void OnDockerHealthCheckUnhealthy(std::wstring const & codePackageName, int64 instanceId);

        Common::AsyncOperationSPtr BeginTerminateFabricTypeHostCodePackage(
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent,
            int64 instanceId);

        Common::ErrorCode EndTerminateFabricTypeHostCodePackage(
            Common::AsyncOperationSPtr const & operation,
            __out int64 & instanceId);

        __declspec(property(get=get_CurrentVersionInstance)) ServiceModel::ServicePackageVersionInstance const CurrentVersionInstance;
        ServiceModel::ServicePackageVersionInstance const get_CurrentVersionInstance() const;

        __declspec(property(get = get_ServicePackage)) ServicePackage2 const & ServicePackageObj;
        ServicePackage2 const & get_ServicePackage() const;

        __declspec(property(get = get_PackageDescription)) ServiceModel::ServicePackageDescription const& PackageDescription;
        ServiceModel::ServicePackageDescription const& get_PackageDescription() const { return packageDescription__; }
    private:
        __declspec(property(get=get_EnvironmentManager)) EnvironmentManagerUPtr const & EnvironmentManagerObj;
        EnvironmentManagerUPtr const & get_EnvironmentManager() const;


    protected:
        virtual void OnAbort();

    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class SwitchAsyncOperation;        

        class CodePackagesAsyncOperationBase;
        class ActivateCodePackagesAsyncOperation;
        class UpdateCodePackagesAsyncOperation;
        class DeactivateCodePackagesAsyncOperation;
        class AbortCodePackagesAsyncOperation;
        class TerminateFabricTypeHostAsyncOperation;
        class RestartCodePackageInstanceAsyncOperation;

        typedef std::function<void(
            Common::AsyncOperationSPtr const & operation, 
            Common::ErrorCode const error, 
            CodePackageMap const & completed, 
            CodePackageMap const & failed)> CodePackagesAsyncOperationCompletedCallback;        

    private:
        Common::ErrorCode WriteCurrentPackageFile(
            ServiceModel::ServicePackageVersion const & packageVersion);
        
        Common::ErrorCode LoadCodePackages(
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            ServiceModel::ServicePackageDescription const & packageDescription,
            ServicePackageInstanceEnvironmentContextSPtr const & envContext,
            __inout CodePackageMap & codePackages);

        Common::ErrorCode CreateImplicitTypeHostCodePackage(
            std::wstring const & codePackageVersion,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            std::vector<std::wstring> const & typeNames,
            ServicePackageInstanceEnvironmentContextSPtr const & envContext,
            __out CodePackageSPtr & typeHostCodePackage);

        void ActivateCodePackagesAsync(
            CodePackageMap const & codePackages,
            Common::TimeSpan const timeout,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
            Common::AsyncOperationSPtr const & parent);

        void FinishAtivateCodePackages(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback);

        void UpdateCodePackagesAsync(
            CodePackageMap const & codePackages,
            ServiceModel::ServicePackageDescription const & newPackageDescription,
            ServiceModel::ServicePackageVersionInstance const & servicePackageVersionInstance,
            Common::TimeSpan const timeout,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
            Common::AsyncOperationSPtr const & parent);

        void FinishUpdateCodePackages(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback);

        void DeactivateCodePackagesAsync(
            CodePackageMap const & codePackages,
            Common::TimeSpan const timeout,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
            Common::AsyncOperationSPtr const & parent);

        void FinishDeactivateCodePackages(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback);

        void AbortCodePackagesAsync(
            CodePackageMap const & codePackages,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback,
            Common::AsyncOperationSPtr const & parent);

        void FinishAbortCodePackages(
            Common::AsyncOperationSPtr const & operation, 
            bool expectedCompletedSynchronously,
            CodePackagesAsyncOperationCompletedCallback const & asyncCallback);

        std::map<std::wstring, std::wstring> GetCodePackagePortMappings(
            ServiceModel::ContainerPoliciesDescription const &,
            ServicePackageInstanceEnvironmentContextSPtr const &);

        ErrorCode TryGetPackageSecurityPrincipalInfo(
            __in std::wstring const & name,
            __out SecurityPrincipalInformationSPtr & info);

    private:
        ServicePackage2Holder const servicePackageHolder_;
        Management::ImageModel::RunLayoutSpecification const runLayout_;        
        std::wstring const failureId_;
        std::wstring const healthPropertyId_;
        std::wstring const componentId_;
        std::vector<ServiceTypeInstanceIdentifier> const serviceTypeInstanceIds_;
        Common::DateTime activationTime_;

        ServiceModel::ServicePackageVersionInstance currentVersionInstance__;        
        ServiceModel::ServicePackageDescription packageDescription__;
        ServicePackageInstanceEnvironmentContextSPtr environmentContext__;
        CodePackageMap codePackages__;
    };
}
