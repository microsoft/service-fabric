// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

class WorkingFolderTestHost :
    public Common::ComponentRoot,
    public Common::FabricComponent,
    public Hosting2::IGuestServiceTypeHost,
    Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
{
    DENY_COPY(WorkingFolderTestHost)

public:
    WorkingFolderTestHost(
        std::wstring const & testName, 
        bool isSetupEntryPoint, 
        bool isActivator);

    virtual ~WorkingFolderTestHost();

    Common::ErrorCode OnOpen();
    Common::ErrorCode OnClose();
    void OnAbort();

public:
    __declspec(property(get = get_ActivationManager)) Common::ComPointer<Hosting2::ComGuestServiceCodePackageActivationManager> & ActivationManager;
    Common::ComPointer<Hosting2::ComGuestServiceCodePackageActivationManager> & get_ActivationManager() override { return activationManager_; }

    __declspec(property(get = get_Endpoints)) std::vector<ServiceModel::EndpointDescription> const & Endpoints;
    std::vector<ServiceModel::EndpointDescription> const & get_Endpoints() override { return endpointDescriptions_; }

    __declspec(property(get = get_DependentCodePackages)) std::vector<std::wstring> const & DependentCodePackages;
    std::vector<std::wstring> const & get_DependentCodePackages() override { return dependentCodePackages_; }

    __declspec(property(get = get_HostContext)) Hosting2::ApplicationHostContext const & HostContext;
    Hosting2::ApplicationHostContext const & get_HostContext()  override { return hostContext_; }

    __declspec(property(get = get_CodeContext)) Hosting2::CodePackageContext const & CodeContext;
    Hosting2::CodePackageContext const & get_CodeContext() override { return codePackageContext_; }

    Common::ErrorCode GetCodePackageActivator(
        _Out_ Common::ComPointer<IFabricCodePackageActivator> & codePackageActivator) override;

private:
    Common::ErrorCode VerifyWorkingFolder();

private:
    Common::ComPointer<IFabricRuntime> runtime_;
    Common::ComPointer<IFabricCodePackageActivationContext2> activationContext_;
    std::wstring traceId_;
    std::wstring const testName_;
    std::wstring const typeName_;
    bool const isSetupEntryPoint_;
    bool const isActivator_;
    Common::RealConsole console_;
    Common::ComPointer<Hosting2::ComGuestServiceCodePackageActivationManager> activationManager_;
    std::vector<ServiceModel::EndpointDescription> endpointDescriptions_;
    std::vector<std::wstring> dependentCodePackages_;
    Hosting2::ApplicationHostContext hostContext_;
    Hosting2::CodePackageContext codePackageContext_;

    static Common::GlobalWString WorkingFolderAppId;
};
