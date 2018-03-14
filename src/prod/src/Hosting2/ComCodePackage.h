// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComCodePackage :
        public IFabricCodePackage2,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComCodePackage)

        BEGIN_COM_INTERFACE_LIST(ComCodePackage)
            COM_INTERFACE_ITEM(IID_IUnknown, IFabricCodePackage)
            COM_INTERFACE_ITEM(IID_IFabricCodePackage, IFabricCodePackage)
            COM_INTERFACE_ITEM(IID_IFabricCodePackage2, IFabricCodePackage2)
        END_COM_INTERFACE_LIST()

    public:
        ComCodePackage(
            ServiceModel::CodePackageDescription const & codePackageDescription,
            std::wstring const& serviceManifestName,
            std::wstring const& serviceManifestVersion, 
            std::wstring const& path,
            ServiceModel::RunAsPolicyDescription const & setupRunAsPolicyDescription,
            ServiceModel::RunAsPolicyDescription const & runAsPolicyDescription);
        virtual ~ComCodePackage();

        const FABRIC_CODE_PACKAGE_DESCRIPTION * STDMETHODCALLTYPE get_Description(void);

        const FABRIC_RUNAS_POLICY_DESCRIPTION * STDMETHODCALLTYPE get_SetupEntryPointRunAsPolicy(void);
        
        const FABRIC_RUNAS_POLICY_DESCRIPTION * STDMETHODCALLTYPE get_EntryPointRunAsPolicy(void);

        LPCWSTR STDMETHODCALLTYPE get_Path(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_CODE_PACKAGE_DESCRIPTION> codePackageDescription_;
        Common::ReferencePointer<FABRIC_RUNAS_POLICY_DESCRIPTION> setupRunAsPolicy_;
        Common::ReferencePointer<FABRIC_RUNAS_POLICY_DESCRIPTION> runAsPolicy_;
        std::wstring path_;
    };
}
