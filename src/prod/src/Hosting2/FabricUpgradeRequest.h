// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class FabricUpgradeRequest : public Serialization::FabricSerializable
    {
    public:
        FabricUpgradeRequest();
        FabricUpgradeRequest(
            bool const useFabricInstallerService,
            std::wstring const & programToRun,
            std::wstring const & arguments,
            std::wstring const & downloadedFabricPackageLocation);
            
        __declspec(property(get=get_ProgramToRun)) std::wstring const & ProgramToRun;
        std::wstring const & get_ProgramToRun() const { return programToRun_; }

        __declspec(property(get=get_Arguments)) std::wstring const & Arguments;
        std::wstring const & get_Arguments() const { return arguments_; }

        __declspec(property(get = get_UseFabricInstallerService_)) bool const UseFabricInstallerService;
        bool const get_UseFabricInstallerService_() const { return useFabricInstallerService_; }

        __declspec(property(get = get_DownloadedFabricPackageLocation)) std::wstring const & DownloadedFabricPackageLocation;
        std::wstring const & get_DownloadedFabricPackageLocation() const { return downloadedFabricPackageLocation_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_04(programToRun_, arguments_, useFabricInstallerService_, downloadedFabricPackageLocation_);

    private:
        std::wstring programToRun_;
        std::wstring arguments_;
        bool useFabricInstallerService_;
        std::wstring downloadedFabricPackageLocation_;
    };
}
