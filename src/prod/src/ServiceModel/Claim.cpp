// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

Claim::Claim() :
    claimType_()
    , issuer_()
    , originalIssuer_()
    , subject_()
    , value_()
    , valueType_()
{
}

Claim::Claim(
    wstring const& claimType,
    wstring const& issuer,
    wstring const& originalIssuer,
    wstring const& subject,
    wstring const& value,
    wstring const& valueType)
    : claimType_(claimType)
      , issuer_(issuer)
      , originalIssuer_(originalIssuer)
      , subject_(subject)
      , value_(value)
      , valueType_(valueType)
{
}

void Claim::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("Claim { ");
    w.Write("ClaimType = {0} ", claimType_);
    w.Write("Issuer = {0} ", issuer_);
    w.Write("OriginalIssuer = {0} ", originalIssuer_);
    w.Write("Subject = {0} ", subject_);
    w.Write("Value = {0} ", value_);
    w.Write("ValueType = {0} ", valueType_);
    w.Write("}");
}

