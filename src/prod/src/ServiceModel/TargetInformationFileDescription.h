// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct Parser;
    struct TargetInformationFileDescription
    {
    public:
        TargetInformationFileDescription();
        TargetInformationFileDescription(TargetInformationFileDescription const & other);
        TargetInformationFileDescription(TargetInformationFileDescription && other);

        TargetInformationFileDescription const & operator = (TargetInformationFileDescription const & other);
        TargetInformationFileDescription const & operator = (TargetInformationFileDescription && other);

        bool operator == (TargetInformationFileDescription const & other) const;
        bool operator != (TargetInformationFileDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        Common::ErrorCode FromXml(std::wstring const & fileName);
        Common::ErrorCode ToXml(std::wstring const & fileName);

        void clear();

    public:
        WindowsFabricDeploymentDescription CurrentInstallation;
        WindowsFabricDeploymentDescription TargetInstallation;

    private:
        friend struct Parser;

        void ReadFromXml(Common::XmlReaderUPtr const &);

        static Common::StringLiteral TargetInformationXmlContent;
        static Common::StringLiteral CurrentInstallationElementForXCopy;
        static Common::StringLiteral TargetInstallationElementForXCopy;
    };
}
