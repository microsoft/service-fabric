// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
	namespace ResourceManager
	{
		class ResourceIdentifier
			: public Serialization::FabricSerializable
			, public Common::IFabricJsonSerializable
		{
		public:

			ResourceIdentifier();
			ResourceIdentifier(std::wstring const & resourceType);
			ResourceIdentifier(std::wstring const & resourceType, std::wstring const & resourceName);
			ResourceIdentifier(ResourceIdentifier const & resourceId);

			static Common::GlobalWString InvalidResourceType;

			__declspec (property(get = get_ResourceName)) std::wstring const & ResourceName;
			std::wstring const & get_ResourceName() const { return this->resourceName_; }

			__declspec (property(get = get_ResourceType)) std::wstring const & ResourceType;
			std::wstring const & get_ResourceType() const { return this->resourceType_; }

			ResourceIdentifier & operator=(ResourceIdentifier const & resourceId) = default;

			bool operator<(ResourceIdentifier const & resourceId) const;
			bool operator>(ResourceIdentifier const & resourceId) const;

			bool operator==(ResourceIdentifier const & other) const;
			bool operator!=(ResourceIdentifier const & other) const;

			bool IsEmpty() const;

			BEGIN_JSON_SERIALIZABLE_PROPERTIES()
				SERIALIZABLE_PROPERTY(L"ResourceName", resourceName_)
				SERIALIZABLE_PROPERTY(L"ResourceType", resourceType_)
			END_JSON_SERIALIZABLE_PROPERTIES()

			FABRIC_FIELDS_02(
				resourceType_,
				resourceName_)

			void WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const & options) const;

		private:
			std::wstring resourceType_;
			std::wstring resourceName_;
		};
	}
}

namespace std
{
	template<>
	struct hash<Management::ResourceManager::ResourceIdentifier>
	{
		std::size_t operator()(Management::ResourceManager::ResourceIdentifier const &) const noexcept;
	};
}

DEFINE_USER_ARRAY_UTILITY(Management::ResourceManager::ResourceIdentifier)
