// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class WinFabStoreLayoutSpecification
        {
            DENY_COPY(WinFabStoreLayoutSpecification)

        public:
            WinFabStoreLayoutSpecification();
            WinFabStoreLayoutSpecification(std::wstring const & storeRoot);

            std::wstring GetPatchFile(std::wstring const & version) const;

            std::wstring GetCabPatchFile(std::wstring const & version) const;

            static std::wstring GetPatchFileName(std::wstring const & version, bool useCabFile = false);

            std::wstring GetCodePackageFolder(std::wstring const & version) const;

            std::wstring GetClusterManifestFile(std::wstring const & clusterManifestVersion) const;

            static std::wstring GetClusterManifestFileName(std::wstring const & clusterManifestVersion);            

            _declspec(property(get=get_Root, put=set_Root)) std::wstring & Root;
            std::wstring const & get_Root() const { return windowsFabRoot_; }

            void set_Root(std::wstring const & value) { windowsFabRoot_.assign(Common::Path::Combine(value, WindowsFabricStore)); }
            
        private:
            std::wstring windowsFabRoot_;
            static Common::GlobalWString WindowsFabricStore;
        };
    }
}
