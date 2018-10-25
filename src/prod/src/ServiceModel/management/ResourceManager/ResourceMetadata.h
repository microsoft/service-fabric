// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
	namespace ResourceManager
	{
		class ResourceMetadata
			: public ServiceModel::ClientServerMessageBody
		{
		public:
			ResourceMetadata();
			ResourceMetadata(std::wstring const & resourceType);

			__declspec (property(get = get_ResourceType)) std::wstring const & ResourceType;
			std::wstring const & get_ResourceType() const { return resourceType_; }

			__declspec (property(get = get_Metadata)) std::map<std::wstring, std::wstring> & Metadata;
			std::map<std::wstring, std::wstring> & get_Metadata() { return this->metadata_; }

			BEGIN_JSON_SERIALIZABLE_PROPERTIES()
				SERIALIZABLE_PROPERTY_CHAIN()
				SERIALIZABLE_PROPERTY(L"ResourceType", resourceType_)
				SERIALIZABLE_PROPERTY(L"Metadata", metadata_)
			END_JSON_SERIALIZABLE_PROPERTIES()

			FABRIC_FIELDS_02(
				resourceType_,
				metadata_)

		private:
			std::map<std::wstring, std::wstring> metadata_;
			std::wstring resourceType_;
		};
	}
}