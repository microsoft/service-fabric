// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ApplicationPackageRef> element.
    struct ApplicationManifestDescription;
    struct ApplicationPackageDescription;
    struct ApplicationInstanceDescription;
    struct ApplicationPackageReference
    {
    public:
        ApplicationPackageReference(); 
        ApplicationPackageReference(ApplicationPackageReference const & other);
        ApplicationPackageReference(ApplicationPackageReference && other);

        ApplicationPackageReference const & operator = (ApplicationPackageReference const & other);
        ApplicationPackageReference const & operator = (ApplicationPackageReference && other);

        bool operator == (ApplicationPackageReference const & other) const;
        bool operator != (ApplicationPackageReference const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    public:        
        RolloutVersion RolloutVersionValue;                            

    private:
        friend struct ApplicationManifestDescription;
        friend struct ApplicationPackageDescription;
        friend struct ApplicationInstanceDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
