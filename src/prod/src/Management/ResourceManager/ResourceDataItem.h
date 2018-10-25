// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
	namespace ResourceManager
	{
		class ResourceDataItem
			: public Store::StoreData
		{
		public:
			ResourceDataItem();
			ResourceDataItem(std::wstring const & name);
			ResourceDataItem(ResourceIdentifier const & resourceId);

			__declspec(property(get = get_Name)) std::wstring const & Name;
			std::wstring const & get_Name() const { return name_; }

			__declspec(property(get = get_References)) std::set<ResourceIdentifier> & References;
			std::set<ResourceIdentifier> & get_References() { return references_; }

			std::wstring const & get_Type() const override { return StoreTypes::Resources; }

			FABRIC_FIELDS_02(name_, references_)

			virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const override;

		protected:
			std::wstring ConstructKey() const override { return name_; }

		private:
			std::wstring  name_;
			std::set<ResourceIdentifier> references_;
		};
	}
}