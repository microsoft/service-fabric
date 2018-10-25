// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{    
    class SingleCodePackageApplicationHost : public ApplicationHost
    {
    public:
        SingleCodePackageApplicationHost(
            ApplicationHostContext const & hostContext,
            std::wstring const & runtimeServiceAddress,
            Common::PCCertContext certContext,
            std::wstring const & serverThumbprint,
            CodePackageContext const & codeContext);
        virtual ~SingleCodePackageApplicationHost();

        static Common::ErrorCode Create(
            ApplicationHostContext const & hostContext,
            std::wstring const & runtimeServiceAddress,
            Common::PCCertContext certContext,
            std::wstring const & serverThumbprint,
            CodePackageContext const & codeContext,
            __out ApplicationHostSPtr & applicationHost);

        void GetCodePackageContext(CodePackageContext & codeContext);

    protected:
        virtual Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const & asyncOperation);

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
        CodePackageContext codeContext_;
        ApplicationHostCodePackageActivatorSPtr codePackageActivator_;
    };
}
