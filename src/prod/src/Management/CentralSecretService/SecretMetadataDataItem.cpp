// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

SecretMetadataDataItem::SecretMetadataDataItem() 
    : SecretDataItemBase()
    , latestVersionNumber_(0)
    , key_(L"")
{}

SecretMetadataDataItem::SecretMetadataDataItem(wstring const & name) 
    : SecretDataItemBase(name)
    , latestVersionNumber_(0)
    , key_(name)
{
    StringUtility::ToLower(key_);
}

SecretMetadataDataItem::SecretMetadataDataItem(Secret const & secret)
    : SecretDataItemBase(secret.Name, secret.Kind, secret.ContentType, secret.Description)
    , latestVersionNumber_(0)
    , key_(secret.Name)
{
    StringUtility::ToLower(key_);
}

SecretMetadataDataItem::SecretMetadataDataItem(SecretReference const & secretRef)
    : SecretMetadataDataItem(secretRef.Name)
{}

SecretMetadataDataItem::~SecretMetadataDataItem()
{
}

wstring const SecretMetadataDataItem::get_LatestVersion() const
{
    static int const VersionLength(10);
    wchar_t str[VersionLength + 1];
    int swprintfResult;

    if (VersionLength != (swprintfResult = swprintf(str, VersionLength + 1, L"%010d", this->latestVersionNumber_)))
    {
        Assert::CodingError("swprintf() returned unexpected value. VersionLength = {0}, ReturnValue = {1}.", VersionLength, swprintfResult);
    }

    return wstring(str);
}

void SecretMetadataDataItem::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("SecretMetadata:{0}", this->Name);
}

wstring SecretMetadataDataItem::ConstructKey() const
{ 
    return key_;
}

SecretReference SecretMetadataDataItem::ToSecretReference() const
{
    return SecretReference(this->Name);
}

Secret SecretMetadataDataItem::ToSecret() const
{
    return Secret(
        this->Name,
        L"",        //  version
        L"",        //  value
        this->Kind,
        this->ContentType,
        this->Description);
}

//
// Verifies whether the current object can be modified to match the object specified by the parameter.
// Consider moving this to the base class if/when secret value objects support updates.
//
bool SecretMetadataDataItem::IsValidUpdate(SecretMetadataDataItem const & update) const
{
    // another item may be considered as an update of this object 
    // iff all fields but the description are equal. Name and kind
    // must be present in the updating request, content type may be
    // omitted. Only the description is retained, so comparisons are
    // case-insensitive.
    return (StringUtility::AreEqualCaseInsensitive(this->Name, update.Name))
        && (StringUtility::AreEqualCaseInsensitive(this->Kind, update.Kind))
        && (update.ContentType.empty() || StringUtility::AreEqualCaseInsensitive(this->ContentType, update.ContentType));
}

//
// Verifies whether the current object can be modified to match the object specified by the parameter.
// Consider moving this to the base class if/when secret value objects support updates.
//
bool SecretMetadataDataItem::TryUpdateTo(SecretMetadataDataItem const & update)
{
    if (!IsValidUpdate(update))
        return false;

    this->Description = update.Description;

    return true;
}
