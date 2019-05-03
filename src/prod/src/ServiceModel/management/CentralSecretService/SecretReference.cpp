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

///
/// Validates whether the secret reference is properly formed. The reference may point to
/// a secret resource or a secret value; if strict validation is in effect, the 'asResource'
/// flag specifies which type this reference corresponds to. For example, strict validation
/// for a secret version reference will fail if the version is empty.
///
ErrorCode SecretReference::Validate(bool strict, bool asResource) const
{
    if (Constants::SecretNameMaxLength < this->Name.size() / sizeof(wchar_t))
    {
        return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The length of the secret reference name is more than {0} characters.", Constants::SecretNameMaxLength));
    }

    if (Constants::SecretVersionMaxLength < this->Version.size() / sizeof(wchar_t))
    {
        return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The length of the secret reference version is more than {0} characters.", Constants::SecretVersionMaxLength));
    }

    if (!this->Version.empty() && !Secret::ValidateCharacters(this->Version))
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"Version contains invalid charaters. Only alphabets (a-z, A-Z), numbers (0-9), hyphen (-) and underscore (_) are allowed.");
    }

    if (!this->Name.empty() && !Secret::ValidateCharacters(this->Name))
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"Name contains invalid charaters. Only alphabets (a-z, A-Z), numbers (0-9), hyphen (-) and underscore (_) are allowed.");
    }

    if (strict)
    {
        if (this->Name.empty())
        {
            return ErrorCode(ErrorCodeValue::SecretInvalid, L"Name may not be empty for this request.");
        }

        // version.empty() must be equal to asResource
        if (!(this->Version.empty() ^ asResource))
        {
            return ErrorCode(ErrorCodeValue::SecretInvalid, wformatString("A version identifier must{0} be specified for this request.", (asResource ? L" not" : L"")));
        }
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
