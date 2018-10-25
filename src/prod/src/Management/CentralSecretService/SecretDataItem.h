// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
	namespace CentralSecretService
	{
		class SecretDataItem : public Store::StoreData
		{
		public:
			SecretDataItem() : StoreData() {}
			SecretDataItem(std::wstring const & key);
			SecretDataItem(std::wstring const & key, Secret & secret);

			SecretDataItem(std::wstring const & name, std::wstring const & version);
			SecretDataItem(SecretReference &);
			SecretDataItem(Secret &);

			virtual ~SecretDataItem();

			SecretDataItem& operator=(Secret &);

			__declspec(property(get = get_Name)) std::wstring const & Name;
			std::wstring const & get_Name() const { return name_; }

			__declspec(property(get = get_Version)) std::wstring const & Version;
			std::wstring const & get_Version() const { return version_; }

			__declspec(property(get = get_Value)) std::wstring const & Value;
			std::wstring const & get_Value() const { return value_; }

			std::wstring const & get_Type() const override { return Constants::StoreType::Secrets; }

			void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const override;

			SecretReference ToSecretReference();
			Secret ToSecret();

			FABRIC_FIELDS_03(
				name_,
				version_,
				value_)

		protected:
			std::wstring ConstructKey() const override { return key_; }

		private:
			std::wstring key_;
			std::wstring name_;
			std::wstring version_;
			std::wstring value_;
		};
	}
}