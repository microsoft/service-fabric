// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
	struct ApplicationServiceDescription;
	struct ApplicationServiceTemplateDescription;
	struct ServicePartitionDescription : public Common::IFabricJsonSerializable
	{
	public:
		ServicePartitionDescription();
		ServicePartitionDescription(PartitionSchemeDescription::Enum, std::wstring, std::wstring, std::wstring, std::vector<std::wstring> const&);
		ServicePartitionDescription(ServicePartitionDescription const & other);
		ServicePartitionDescription(ServicePartitionDescription && other);

		ServicePartitionDescription const & operator = (ServicePartitionDescription const & other);
		ServicePartitionDescription const & operator = (ServicePartitionDescription && other);

		bool operator == (ServicePartitionDescription const & other) const;
		bool operator != (ServicePartitionDescription const & other) const;

		void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;

		void clear();

	public:

		BEGIN_JSON_SERIALIZABLE_PROPERTIES()
			SERIALIZABLE_PROPERTY_ENUM(Constants::PartitionScheme, Scheme)
			SERIALIZABLE_PROPERTY_IF(Constants::Count, PartitionCount, ((Scheme == PartitionSchemeDescription::Enum::Named) || (Scheme == PartitionSchemeDescription::Enum::UniformInt64Range)))
			SERIALIZABLE_PROPERTY_IF(Constants::Names, PartitionNames, Scheme == PartitionSchemeDescription::Enum::Named)
			SERIALIZABLE_PROPERTY_IF(Constants::LowKey, LowKey, Scheme == PartitionSchemeDescription::Enum::UniformInt64Range)
			SERIALIZABLE_PROPERTY_IF(Constants::HighKey, HighKey, Scheme == PartitionSchemeDescription::Enum::UniformInt64Range)
			END_JSON_SERIALIZABLE_PROPERTIES()

			// Describes the type of partition - Singleton, Uniform or Named
			PartitionSchemeDescription::Enum Scheme;

		// UniformInt64Partition
		std::wstring PartitionCount;
		std::wstring LowKey;
		std::wstring HighKey;

		// NamedPartition
		std::vector<std::wstring> PartitionNames;

	private:
		friend struct ApplicationServiceDescription;
		friend struct ApplicationServiceTemplateDescription;

		void ReadFromXml(Common::XmlReaderUPtr const &, PartitionSchemeDescription::Enum);
		void ReadSingletonPartitionFromXml(Common::XmlReaderUPtr const & xmlReader);
		void ReadUniformInt64PartitionFromXml(Common::XmlReaderUPtr const & xmlReader);
		void ReadNamedPartitionFromXml(Common::XmlReaderUPtr const & xmlReader);


		Common::ErrorCode WriteToXml(Common::XmlWriterUPtr const & xmlWriter);
	};
}
