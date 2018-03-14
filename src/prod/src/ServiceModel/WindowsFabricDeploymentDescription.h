// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct TargetInformationFileDescription;
    struct WindowsFabricDeploymentDescription
    {
    public:
        WindowsFabricDeploymentDescription();
        WindowsFabricDeploymentDescription(WindowsFabricDeploymentDescription const & other);
        WindowsFabricDeploymentDescription(WindowsFabricDeploymentDescription && other);

        WindowsFabricDeploymentDescription const & operator = (WindowsFabricDeploymentDescription const & other);
        WindowsFabricDeploymentDescription const & operator = (WindowsFabricDeploymentDescription && other);

        bool operator == (WindowsFabricDeploymentDescription const & other) const;
        bool operator != (WindowsFabricDeploymentDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring InstanceId;
        std::wstring MSILocation;
        std::wstring ClusterManifestLocation;
        std::wstring InfrastructureManifestLocation;
        std::wstring TargetVersion;
        std::wstring NodeName;
        BOOLEAN RemoveNodeState;
        std::wstring UpgradeEntryPointExe;
        std::wstring UpgradeEntryPointExeParameters;
        std::wstring UndoUpgradeEntryPointExe;
        std::wstring UndoUpgradeEntryPointExeParameters;

        __declspec (property(get = get_IsValid)) bool const & IsValid;
        bool const & get_IsValid() const { return this->isValid_; }

    private:
        friend struct TargetInformationFileDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &, std::wstring const & startElement);

        void ReadAttributeIfExists(Common::XmlReaderUPtr const &, std::wstring const & attributeName, std::wstring & value, std::wstring const & defaultValue = L"");

    private:
        bool isValid_;
    };
}
