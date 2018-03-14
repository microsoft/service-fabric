// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    // <ServicePackageRef> element.
    struct ApplicationInstanceDescription;
    struct ServicePackageReference
    {
    public:
        ServicePackageReference(); 
        ServicePackageReference(ServicePackageReference const & other);
        ServicePackageReference(ServicePackageReference && other);

        ServicePackageReference const & operator = (ServicePackageReference const & other);
        ServicePackageReference const & operator = (ServicePackageReference && other);

        bool operator == (ServicePackageReference const & other) const;
        bool operator != (ServicePackageReference const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void clear();

    public:
        std::wstring Name;        
        RolloutVersion RolloutVersionValue;                            

    private:
        friend struct ApplicationInstanceDescription;

        void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
    };
}
