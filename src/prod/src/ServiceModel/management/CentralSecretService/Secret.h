// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
	namespace CentralSecretService
	{
		class Secret : public ServiceModel::ClientServerMessageBody
		{
		public:
			Secret();
			Secret(std::wstring const & name);

			Secret(std::wstring const & name, std::wstring const & version, std::wstring const & value);
			virtual ~Secret();

			__declspec (property(get = get_Name)) std::wstring const & Name;
			std::wstring const & get_Name() const { return name_; }

			__declspec (property(get = get_Version)) std::wstring const & Version;
			std::wstring const & get_Version() const { return version_; }

			__declspec (property(get = get_Value, put = set_Value)) std::wstring const & Value;
			std::wstring const & get_Value() const { return value_; }
			void set_Value(std::wstring const & value) { this->value_ = value; }

			Common::ErrorCode FromPublicApi(__in FABRIC_SECRET const & secret);
			Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_SECRET & secret) const;
			Common::ErrorCode Validate() const;

			void WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const & options) const;

			std::wstring ToResourceName() const;

			BEGIN_JSON_SERIALIZABLE_PROPERTIES()
				SERIALIZABLE_PROPERTY_CHAIN()
				SERIALIZABLE_PROPERTY(L"Name", name_)
				SERIALIZABLE_PROPERTY(L"Version", version_)
				SERIALIZABLE_PROPERTY_IF(L"Value", value_, !value_.empty())
			END_JSON_SERIALIZABLE_PROPERTIES()

			FABRIC_FIELDS_03(
				name_,
				version_,
				value_)

		private:
			std::wstring name_;
			std::wstring version_;
			std::wstring value_;
		};
	}
}

DEFINE_USER_ARRAY_UTILITY(Management::CentralSecretService::Secret)