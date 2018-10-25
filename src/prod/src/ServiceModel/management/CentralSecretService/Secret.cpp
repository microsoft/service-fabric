// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Naming;
using namespace ServiceModel;
using namespace std;

using namespace Management::CentralSecretService;

Secret::Secret()
	: ClientServerMessageBody()
	, name_()
	, version_()
{
}

Secret::Secret(std::wstring const & name)
	: ClientServerMessageBody()
	, name_(name)
	, version_()
{
}

Secret::Secret(std::wstring const & name,
	std::wstring const & version,
	std::wstring const & value)
	: ClientServerMessageBody()
	, name_(name)
	, version_(version)
	, value_(value)
{
}

Secret::~Secret()
{
	SecureZeroMemory((void *)this->value_.c_str(), this->value_.size() * sizeof(wchar_t));
}

ErrorCode Secret::FromPublicApi(__in FABRIC_SECRET const & secret)
{
	ErrorCode error = StringUtility::LpcwstrToWstring2(secret.Name, false, this->name_);

	if (!error.IsSuccess())
	{
		return error;
	}
	error = StringUtility::LpcwstrToWstring2(secret.Version, false, this->version_);

	if (!error.IsSuccess())
	{
		return error;
	}
	error = StringUtility::LpcwstrToWstring2(secret.Value, true, this->value_);

	if (!error.IsSuccess())
	{
		return error;
	}

	return ErrorCode::Success();
}

ErrorCode Secret::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SECRET & secret) const
{
	secret.Name = heap.AddString(this->name_);
	secret.Version = heap.AddString(this->version_);
	secret.Value = heap.AddString(this->value_, true);

	return ErrorCode::Success();
}

ErrorCode Secret::Validate() const
{
	if (Constants::SecretNameMaxLength < this->Name.size() / sizeof(wchar_t))
	{
		return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The length of the secret name is more than {0} characters.", Constants::SecretNameMaxLength));
	}

	if (Constants::SecretVersionMaxLength < this->Version.size() / sizeof(wchar_t))
	{
		return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The length of the secret version is more than {0} characters.", Constants::SecretNameMaxLength));
	}

	if (Constants::SecretValueMaxSize < this->Value.size())
	{
		return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The size of the secret value is more than {0} bytes.", Constants::SecretNameMaxLength));
	}

	return ErrorCode::Success();
}


void Secret::WriteTo(__in TextWriter & writer, FormatOptions const & options) const
{
	UNREFERENCED_PARAMETER(options);

	writer.Write("Secret:{0}#{1}#", this->Name, this->Version);
}

wstring Secret::ToResourceName() const
{
	return wformatString(ServiceModelConstants::SecretResourceNameFormat, this->Name, this->Version);
}
