// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
   class NonActivatedApplicationHost :
        public ApplicationHost
    {
    public:
        NonActivatedApplicationHost(
            std::wstring const & hostId, 
            KtlSystem *,
            std::wstring const & runtimeServiceAddress);
        virtual ~NonActivatedApplicationHost();

        static Common::ErrorCode NonActivatedApplicationHost::Create(
            KtlSystem *,
            std::wstring const & runtimeServiceAddress,
            __out ApplicationHostSPtr & applicationHost);

        static Common::ErrorCode NonActivatedApplicationHost::Create2(
            KtlSystem *,
            std::wstring const & runtimeServiceAddress,
            std::wstring const & hostId,
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
   };
}

