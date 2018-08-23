// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class IGuestServiceTypeHost
    {
    public:
        __declspec(property(get = get_ActivationManager)) Common::ComPointer<ComGuestServiceCodePackageActivationManager> & ActivationManager;
        virtual Common::ComPointer<ComGuestServiceCodePackageActivationManager> & get_ActivationManager() = 0;

        __declspec(property(get = get_Endpoints)) std::vector<ServiceModel::EndpointDescription> const & Endpoints;
        virtual std::vector<ServiceModel::EndpointDescription> const & get_Endpoints() = 0;

        __declspec(property(get = get_DependentCodePackages)) std::vector<std::wstring> const & DependentCodePackages;
        virtual std::vector<std::wstring> const & get_DependentCodePackages() = 0;

        __declspec(property(get = get_HostContext)) ApplicationHostContext const & HostContext;
        virtual ApplicationHostContext const & get_HostContext() = 0;

        __declspec(property(get = get_CodeContext)) CodePackageContext const & CodeContext;
        virtual CodePackageContext const & get_CodeContext() = 0;

        virtual Common::ErrorCode GetCodePackageActivator(_Out_ Common::ComPointer<IFabricCodePackageActivator> & codePackageActivator) = 0;
    };
}
