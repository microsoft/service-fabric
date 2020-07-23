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
    , value_()
    , kind_()
    , contentType_()
    , description_()
{
}

Secret::Secret(std::wstring const & name)
    : ClientServerMessageBody()
    , name_(name)
    , version_()
    , value_()
    , kind_()
    , contentType_()
    , description_()
{
}

Secret::Secret(std::wstring const & name,
    std::wstring const & version,
    std::wstring const & value,
    std::wstring const & kind,
    std::wstring const & contentType,
    std::wstring const & description)
    : ClientServerMessageBody()
    , name_(name)
    , version_(version)
    , value_(value)
    , kind_(kind)
    , contentType_(contentType)
    , description_(description)
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

    error = StringUtility::LpcwstrToWstring2(secret.Kind, true, this->kind_);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = StringUtility::LpcwstrToWstring2(secret.ContentType, true, this->contentType_);
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
    secret.Kind = heap.AddString(this->kind_);
    secret.ContentType = heap.AddString(this->contentType_);

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
        return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The length of the secret version is more than {0} characters.", Constants::SecretVersionMaxLength));
    }

    if (Constants::SecretValueMaxSize < this->Value.size())
    {
        return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The size of the secret value is more than {0} bytes.", Constants::SecretValueMaxSize));
    }

    if (Constants::SecretKindMaxLength < this->Kind.size() / sizeof(wchar_t))
    {
        return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The size of the secret kind is more than {0} characters.", Constants::SecretKindMaxLength));
    }

    if (Constants::SecretContentTypeMaxLength < this->ContentType.size() / sizeof(wchar_t))
    {
        return ErrorCode(ErrorCodeValue::PropertyTooLarge, wformatString("The size of the secret content type is more than {0} characters.", Constants::SecretContentTypeMaxLength));
    }

    if (this->Name.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"Name must be provided.");
    }

    if (!Secret::ValidateCharacters(this->Name))
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"Name contains invalid charaters. Only alphabets (a-z, A-Z), numbers (0-9), hyphen (-) and underscore (_) are allowed.");
    }

    // any additional validation depends on the type of object being stored - this assumes the method 
    // is only invoked on incoming objects. The object must validate as exactly one type - either secret
    // resource or a versioned value. The 'value' field is used as an indication of the caller's intent - 
    // if a value was specified, the object will be validated as an attempt to add a versioned value to
    // an existing secret resource.
    auto evalAsSecretResource = ValidateAsSecretResource(*this);
    auto isSecretResource = evalAsSecretResource.IsSuccess();

    auto evalAsSecretValue = ValidateAsVersionedSecretValue(*this);
    auto isSecretValue = evalAsSecretValue.IsSuccess();
    if (!(isSecretResource ^ isSecretValue))
    {
        if (this->Value.empty())
            return evalAsSecretResource;

        return evalAsSecretValue;
    }

    return ErrorCode::Success();
}

///
/// Attempts to validate the specified Secret object as representing the top-level secret resource. 
///
Common::ErrorCode Secret::ValidateAsSecretResource(Secret const & secret)
{
    // Validation logic:
    //  - name, kind must be non-empty
    //  - value and version must be empty
    //  - content type and description may or may not be set
    if (secret.Name.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"'Name' may not be empty.");
    }

    if (secret.Kind.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"Kind must be provided for secret resources.");
    }

    if (!secret.Version.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"A version identifier may not be specified for a secret resource.");
    }

    if (!secret.Value.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"A value may not be specified for a secret resource.");
    }

    return ErrorCode::Success();
}

///
/// Attempts to validate the specified Secret object as representing a versioned value of a secret resource.
///
Common::ErrorCode Secret::ValidateAsVersionedSecretValue(Secret const & secret)
{
    // Validation logic:
    //  - kind, content type and description must be empty
    //  - name, value and version must be non-empty
    //  - version may not be equal to a restricted, pre-reserved value; case sensitive only
    if (secret.Name.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"'Name' may not be empty.");
    }
    
    if (secret.Version.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"Versioned secret values must provide a version identifier.");
    }
    
    if (secret.Value.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"A secret value may not be empty.");
    }
    
    if (!secret.Kind.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"The secret kind may not be specified for secret values; it must be set on the secret resource.");
    }

    if (!secret.ContentType.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"The content type may not be specified for secret values; it must be set on the secret resource.");
    }

    if (!secret.Description.empty())
    {
        return ErrorCode(ErrorCodeValue::SecretInvalid, L"The description may not be specified for secret values; it must be set on the secret resource.");
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

#ifdef PLATFORM_UNIX
static wregex const NameRegex(L"^[a-zA-Z0-9\-\_]+$");
#else
static wregex const NameRegex(L"^[a-zA-Z0-9\\-\\_]+$");
#endif

bool Secret::ValidateCharacters(wstring const & secretName)
{
    return std::regex_match(secretName, NameRegex);
}
