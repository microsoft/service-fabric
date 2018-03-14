// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

SecurityPrincipalInformation::SecurityPrincipalInformation() 
    : name_(),
    principalId_(),
    sid_(),
    isLocal_(false)
{
}

SecurityPrincipalInformation::SecurityPrincipalInformation(
    wstring const & name,
    wstring const & principalId,
    wstring const & sid,
    bool isLocal) 
    : name_(name),
    principalId_(principalId),
    sid_(sid),
    isLocal_(isLocal)
{
}

SecurityPrincipalInformation::SecurityPrincipalInformation(
    SecurityPrincipalInformation const & other) 
    : name_(other.name_),
    principalId_(other.principalId_),
    sid_(other.sid_),
    isLocal_(other.isLocal_)
{
}

SecurityPrincipalInformation::SecurityPrincipalInformation(SecurityPrincipalInformation && other) 
    : name_(move(other.name_)),
    principalId_(move(other.principalId_)),
    sid_(move(other.sid_)),
    isLocal_(other.isLocal_)
{
}

SecurityPrincipalInformation const & SecurityPrincipalInformation::operator = (SecurityPrincipalInformation const & other)
{
    if (this != &other)
    {
        this->name_ = other.name_;
        this->principalId_ = other.principalId_;
        this->sid_ = other.sid_;
        this->isLocal_ = other.isLocal_;
    }

    return *this;
}

SecurityPrincipalInformation const & SecurityPrincipalInformation::operator = (SecurityPrincipalInformation && other)
{
    if (this != &other)
    {
        this->name_ = move(other.name_);
        this->principalId_ = move(other.principalId_);
        this->sid_ = move(other.sid_);
        this->isLocal_ = other.isLocal_;
    }

    return *this;
}

bool SecurityPrincipalInformation::operator == (SecurityPrincipalInformation const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->name_, other.name_);
    if (!equals) { return equals; }

    equals = StringUtility::AreEqualCaseInsensitive(this->principalId_, other.principalId_);
    if(!equals) { return equals; }
    
    equals = StringUtility::AreEqualCaseInsensitive(this->sid_, other.sid_);
    if(!equals) { return equals; }
    equals = this->isLocal_ == other.isLocal_;
    if (!equals) { return equals; }

    return equals;
}

bool SecurityPrincipalInformation::operator != (SecurityPrincipalInformation const & other) const
{
    return !(*this == other);
}

void SecurityPrincipalInformation::WriteTo(__in TextWriter & w, FormatOptions const &) const
{
    w.Write("SecurityPrincipalInformation { ");
    w.Write("Name = {0}, ", name_);
    w.Write("PrincipalId = {0}", principalId_);
    w.Write("Sid = {0}", sid_);
    w.Write("IsLocalUser = {0}", isLocal_);
    w.Write("}");
}

