// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
	struct ApplicationInstanceDescription;
	struct ApplicationDefaultServiceDescription
	{
	public:
		ApplicationDefaultServiceDescription();
		ApplicationDefaultServiceDescription(ApplicationDefaultServiceDescription const & other);
		ApplicationDefaultServiceDescription(ApplicationDefaultServiceDescription && other);

		ApplicationDefaultServiceDescription const & operator = (ApplicationDefaultServiceDescription const & other);
		ApplicationDefaultServiceDescription const & operator = (ApplicationDefaultServiceDescription && other);

		bool operator == (ApplicationDefaultServiceDescription const & other) const;
		bool operator != (ApplicationDefaultServiceDescription const & other) const;

		void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

		void clear();

	public:
		std::wstring Name;
        std::wstring PackageActivationMode;
		std::wstring ServiceDnsName;
		ApplicationServiceTemplateDescription DefaultService;

	private:
		friend struct ApplicationInstanceDescription;
		friend struct ApplicationManifestDescription;

		void ReadFromXml(Common::XmlReaderUPtr const &);
		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const &);
	};
}
