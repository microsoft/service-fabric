// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <DebugParameters> element.
    struct DebugParametersDescription
    {
    public:
        DebugParametersDescription(); 

        DebugParametersDescription(DebugParametersDescription const & other) = default;
        DebugParametersDescription(DebugParametersDescription && other) = default;

        DebugParametersDescription const & operator = (DebugParametersDescription const & other);
        DebugParametersDescription const & operator = (DebugParametersDescription && other);

        bool operator == (DebugParametersDescription const & other) const;
        bool operator != (DebugParametersDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring ExePath;
        std::wstring Arguments;
        std::wstring CodePackageLinkFolder;
        std::wstring ConfigPackageLinkFolder;
        std::wstring DataPackageLinkFolder;
        std::wstring LockFile;
        std::wstring WorkingFolder;
        std::wstring DebugParametersFile;
        std::wstring EnvironmentBlock;
        RunAsPolicyTypeEntryPointType::Enum EntryPointType;
        std::vector<std::wstring> ContainerEntryPoints;
        std::vector<std::wstring> ContainerMountedVolumes;
        std::vector<std::wstring> ContainerEnvironmentBlock;
        std::vector<std::wstring> ContainerLabels;
        bool DisableReliableCollectionReplicationMode;

        void ReadFromXml(Common::XmlReaderUPtr const &);
        Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);

    private:

        static bool AreEqual(
            std::vector<std::wstring> const & first,
            std::vector<std::wstring> const & second);

        friend struct DigestedCodePackageDescription;
        friend struct ServicePackagePoliciesDescription;
    };
}
