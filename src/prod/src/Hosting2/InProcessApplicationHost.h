// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{   
    class InProcessApplicationHost : public ApplicationHost
    {
    public:
        InProcessApplicationHost(
            GuestServiceTypeHost & guestServiceTypeHost,
            std::wstring const & runtimeServiceAddress,
            ApplicationHostContext const & hostContext,
            CodePackageContext const & codePackageContext);

        virtual ~InProcessApplicationHost();

        static Common::ErrorCode Create(
            GuestServiceTypeHost & guestServiceTypeHost,
            std::wstring const & runtimeServiceAddress,
            ApplicationHostContext const & hostContext,
            CodePackageContext const & codePackageContext,
            __out InProcessApplicationHostSPtr & applicationHost);

        void GetCodePackageContext(CodePackageContext & codeContext);

        void ProcessCodePackageEvent(CodePackageEventDescription eventDescription);

    public:
        __declspec(property(get = get_Hosting)) HostingSubsystem & Hosting;
        inline HostingSubsystem & get_Hosting() const { return guestServiceTypeHost_.Hosting; }

    protected:
        virtual Common::ErrorCode OnCreateAndAddFabricRuntime(
            FabricRuntimeContextUPtr const & fabricRuntimeContextUPtr,
            Common::ComPointer<IFabricProcessExitHandler> const & fabricExitHandler,
            __out FabricRuntimeImplSPtr & fabricRuntime);

        virtual Common::ErrorCode OnGetCodePackageActivationContext(
            CodePackageContext const & codeContext,
            __out CodePackageActivationContextSPtr & codePackageActivationContext);

        virtual Common::ErrorCode OnUpdateCodePackageContext(
            CodePackageContext const & codeContext);

        virtual Common::ErrorCode OnGetCodePackageActivator(
            _Out_ ApplicationHostCodePackageActivatorSPtr & codePackageActivator) override;

        virtual Common::ErrorCode OnCodePackageEvent(
            CodePackageEventDescription const & eventDescription) override;

    private:
        Common::RwLock codeContextLock_;
        CodePackageContext codePackageContext_;
        GuestServiceTypeHost & guestServiceTypeHost_;
        ApplicationHostCodePackageActivatorSPtr codePackageActivator_;
    };
}
