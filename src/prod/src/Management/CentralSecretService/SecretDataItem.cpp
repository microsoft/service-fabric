// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Management::CentralSecretService;

SecretDataItem::SecretDataItem() 
    : SecretDataItemBase()
    , version_()
    , value_()
{
}

SecretDataItem::SecretDataItem(std::wstring const & name, std::wstring const & version)
    : SecretDataItemBase(name)
    , version_(version)
    , value_()
{
}

SecretDataItem::SecretDataItem(
    std::wstring const & name, 
    std::wstring const & version, 
    std::wstring const & value,
    std::wstring const & kind, 
    std::wstring const & contentType,
    std::wstring const & description)
    : SecretDataItemBase(name, kind, contentType, description)
    , version_(version)
    , value_(value)
{}

SecretDataItem::SecretDataItem(SecretReference const & secretRef)
    : SecretDataItem(secretRef.Name, secretRef.Version)
{
}

SecretDataItem::SecretDataItem(Secret const & secret)
    : SecretDataItemBase(secret.Name, secret.Kind, secret.ContentType, secret.Description)
    , version_(secret.Version)
    , value_(secret.Value)
{
}

SecretDataItem::~SecretDataItem()
{
    SecureZeroMemory((void *)this->value_.c_str(), this->value_.size() * sizeof(wchar_t));
}

void SecretDataItem::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("Secret:{1}#{2}#[{0}]", this->Key, this->Name, this->Version);
}

SecretReference SecretDataItem::ToSecretReference() const
{
    return SecretReference(this->Name, this->Version);
}

Secret SecretDataItem::ToSecret() const
{
    return Secret(
        this->Name,
        this->Version,
        this->Value,
        this->Kind,
        this->ContentType,
        this->Description);
}

wstring SecretDataItem::ConstructKey() const
{
    return SecretDataItem::ToKeyName(this->Name, this->version_);
}

static const wstring KeyNameFormat(L"{0}::{1}");

wstring SecretDataItem::ToKeyName(wstring const & secretName, wstring const & secretVersion)
{
    return wformatString(KeyNameFormat, secretName, secretVersion);
}
