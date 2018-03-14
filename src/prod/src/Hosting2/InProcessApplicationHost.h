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
            std::wstring const & hostId, 
            std::wstring const & runtimeServiceAddress,
            CodePackageContext const & codePackageContext);

        virtual ~InProcessApplicationHost();

        static Common::ErrorCode Create(
            std::wstring const & hostId, 
            std::wstring const & runtimeServiceAddress,
            CodePackageContext const & codePackageContext,
            __out ApplicationHostSPtr & applicationHost);

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

    private:
        Common::RwLock codeContextLock_;
        CodePackageContext codePackageContext_;
    };
}
