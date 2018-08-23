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

SecretReference::SecretReference()
	: ClientServerMessageBody()
	, name_()
	, version_()
{
}

SecretReference::SecretReference(std::wstring const & name)
	: ClientServerMessageBody()
	, name_(name)
	, version_()
{
}

SecretReference::SecretReference(std::wstring const & name, std::wstring const & version)
	: ClientServerMessageBody()
	, name_(name)
	, version_(version)
{
}

ErrorCode SecretReference::FromPublicApi(__in FABRIC_SECRET_REFERENCE const & reference)
{
	ErrorCode error = StringUtility::LpcwstrToWstring2(reference.Name, false, this->name_);
	if (!error.IsSuccess())
	{
		return error;
	}

	error = StringUtility::LpcwstrToWstring2(reference.Version, true, this->version_);
	if (!error.IsSuccess())
	{
		return error;
	}

	return ErrorCode::Success();
}

ErrorCode SecretReference::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_SECRET_REFERENCE & reference) const
{
	reference.Name = heap.AddString(this->name_);
	reference.Version = heap.AddString(this->version_, true);

	return ErrorCode::Success();
}

ErrorCode SecretReference::Validate() const
{
	if (Constants::SecretNameMaxLength < this->Name.size() / sizeof(wchar_t))
	{
		return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The length of the secret reference name is more than {0} characters.", Constants::SecretNameMaxLength));
	}

	if (Constants::SecretVersionMaxLength < this->Version.size() / sizeof(wchar_t))
	{
		return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The length of the secret reference version is more than {0} characters.", Constants::SecretNameMaxLength));
	}

	return ErrorCode::Success();
}

void SecretReference::WriteTo(__in TextWriter & writer, FormatOptions const & options) const
{
	UNREFERENCED_PARAMETER(options);

	writer.Write("SecretRef:{0}#{1}#", this->Name, this->Version);
}

wstring SecretReference::ToResourceName() const
{
	return wformatString(ServiceModelConstants::SecretResourceNameFormat, this->Name, this->Version);
}

ErrorCode SecretReference::FromResourceName(
	wstring const & resourceName,
	SecretReference & secretRef)
{
	size_t pos = resourceName.find_first_of(ServiceModelConstants::SecretResourceNameSeparator);

	if (pos == std::wstring::npos)
	{
		return ErrorCode(ErrorCodeValue::InvalidArgument,
			wformatString(
				L"Resource name is not a valid secret resource name (" + ServiceModelConstants::SecretResourceNameFormat + L").",
				"<SecretName>",
				"<SecretVersion>"));
	}

	secretRef = SecretReference(resourceName.substr(0, pos), resourceName.substr(pos + ServiceModelConstants::SecretResourceNameSeparator.size()));
	return ErrorCode::Success();
}
