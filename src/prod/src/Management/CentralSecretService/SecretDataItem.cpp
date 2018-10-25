// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Management::CentralSecretService;

SecretDataItem::SecretDataItem(wstring const & key) : StoreData(), key_(key)
{

}

SecretDataItem::SecretDataItem(std::wstring const & key, Secret & secret) : SecretDataItem(key)
{
    *this = secret;
}

SecretDataItem::SecretDataItem(std::wstring const & name, std::wstring const & version) : StoreData()
{
    if (!version.empty())
    {
        this->key_ = name + L"::" + version;
    }
    else
    {
        this->key_ = name;
    }
}

SecretDataItem::SecretDataItem(SecretReference & secretRef) : SecretDataItem(secretRef.Name, secretRef.Version)
{
}

SecretDataItem::SecretDataItem(Secret & secret) : SecretDataItem(secret.Name, secret.Version)
{
    *this = secret;
}

SecretDataItem::~SecretDataItem()
{
    SecureZeroMemory((void *)this->value_.c_str(), this->value_.size() * sizeof(wchar_t));
}

SecretDataItem& SecretDataItem::operator=(Secret & secret)
{
    this->name_ = secret.Name;
    this->version_ = secret.Version;
    this->value_ = secret.Value;

    return *this;
}

void SecretDataItem::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("Secret:{1}#{2}#[{0}]", this->Key, this->Name, this->Version);
}

SecretReference SecretDataItem::ToSecretReference()
{
    return SecretReference(this->Name, this->Version);
}

Secret SecretDataItem::ToSecret()
{
    return Secret(this->Name, this->Version, this->Value);
}