// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ComDataPackage :
        public IFabricDataPackage,
        private Common::ComUnknownBase
    {
        DENY_COPY(ComDataPackage)

        COM_INTERFACE_LIST1(
            ComDataPackage,
            IID_IFabricDataPackage,
            IFabricDataPackage)

    public:
        ComDataPackage(
            ServiceModel::DataPackageDescription const & dataPackageDescription,
            std::wstring const& serviceManifestName,
            std::wstring const& serviceManifestVersion, 
            std::wstring const& path);
        ~ComDataPackage();

        const FABRIC_DATA_PACKAGE_DESCRIPTION * STDMETHODCALLTYPE get_Description(void);

        LPCWSTR STDMETHODCALLTYPE get_Path(void);

    private:
        Common::ScopedHeap heap_;
        Common::ReferencePointer<FABRIC_DATA_PACKAGE_DESCRIPTION> dataPackageDescription_;
        std::wstring path_;
    };
}
