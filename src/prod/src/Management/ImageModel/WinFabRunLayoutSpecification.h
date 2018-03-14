// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class WinFabRunLayoutSpecification
        {
            DENY_COPY(WinFabRunLayoutSpecification)

        public:
            WinFabRunLayoutSpecification();
            WinFabRunLayoutSpecification(std::wstring const & root);

            std::wstring GetPatchFile(std::wstring const & version) const;

            std::wstring GetCabPatchFile(std::wstring const & version) const;

            std::wstring GetCodePackageFolder(std::wstring const & version) const;

            std::wstring GetClusterManifestFile(std::wstring const & clusterManifestVersion) const ;            

            _declspec(property(get=get_Root, put=set_Root)) std::wstring & Root;
            std::wstring const & get_Root() const { return windowsFabRoot_; }
            void set_Root(std::wstring const & value) { windowsFabRoot_.assign(value); }            
            
        private:
			std::wstring GetPatchFileName(std::wstring const &version, bool useCabFile) const;
            std::wstring windowsFabRoot_;            
        };
    }
}
