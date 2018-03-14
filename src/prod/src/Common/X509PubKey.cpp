// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

#ifdef PLATFORM_UNIX

X509PubKey::X509PubKey(PCCertContext certContext)
{
    auto* x509PubKey = X509_get_X509_PUBKEY(certContext);
    algorithm_ = X509_ALGOR_dup(x509PubKey->algor);
    pubKey_ = ASN1_STRING_dup(x509PubKey->public_key);
}

X509PubKey::~X509PubKey()
{
    //okay to call the following free functions on null pointers
    X509_ALGOR_free(algorithm_);
    ASN1_STRING_free(pubKey_);
}

bool X509PubKey::operator == (X509Identity const & rhs) const
{
    auto& other = (X509PubKey const&)rhs;
    return
        //comment out algoritm comparison for now due to a bug of libssl-dev on Ubuntu
        //LINUXTODO enable algorithm comparison
        //(X509_ALGOR_cmp(algorithm_, other.algorithm_) == 0) &&
        (ASN1_STRING_cmp(pubKey_, other.pubKey_) == 0);
}

bool X509PubKey::operator < (X509Identity const & rhs) const
{
    auto& other = (X509PubKey const&)rhs;
    return ASN1_STRING_cmp(pubKey_, other.pubKey_) < 0;
}

void X509PubKey::WriteTo(Common::TextWriter & w, Common::FormatOptions const & option) const
{
    const int threshold = 20;
    auto length = ASN1_STRING_length(pubKey_);
    if ((length > threshold) && (option.formatString != "l"))
    {
        length = threshold;
    }

    auto data = ASN1_STRING_data(pubKey_);
    w.Write("({0:l})", const_buffer(data, length));
}

#else

X509PubKey::X509PubKey(PCCertContext certContext)
{
    auto & keyInfo = certContext->pCertInfo->SubjectPublicKeyInfo;

    algObjId_ = string(keyInfo.Algorithm.pszObjId);

    paramBlob_.resize(keyInfo.Algorithm.Parameters.cbData);
    KMemCpySafe(
        paramBlob_.data(),
        paramBlob_.size(),
        keyInfo.Algorithm.Parameters.pbData,
        keyInfo.Algorithm.Parameters.cbData);

    keyBlob_.resize(keyInfo.PublicKey.cbData);
    KMemCpySafe(
        keyBlob_.data(),
        keyBlob_.size(),
        keyInfo.PublicKey.pbData,
        keyInfo.PublicKey.cbData);
}

bool X509PubKey::operator == (X509Identity const & rhs) const
{
    auto& other = (X509PubKey const&)rhs;
    return
        (IdType() == other.IdType()) &&
        (algObjId_ == other.algObjId_) &&
        (paramBlob_ == other.paramBlob_) &&
        (keyBlob_ == other.keyBlob_); //unused bits are cleared at initialization
}

bool X509PubKey::operator < (X509Identity const & rhs) const
{
    auto& other = (X509PubKey const&)rhs;

    if (IdType() < rhs.IdType()) return true;
    if (IdType() > rhs.IdType()) return false;

    // IdType() == other.IdType()

    if (StringUtility::Compare(algObjId_, other.algObjId_) < 0) return true;
    if (StringUtility::Compare(algObjId_, other.algObjId_) > 0) return false;

    // algObjId_ == other.algObjId_

    if (paramBlob_ < other.paramBlob_) return true;
    if (paramBlob_ > other.paramBlob_) return false;

    // paramBlob_ == other.paramBlob_

    return keyBlob_ < other.keyBlob_; //unused bits are cleared at initialization
}

void X509PubKey::WriteTo(Common::TextWriter & w, Common::FormatOptions const & option) const
{
    const int threshold = 20;
    auto length = keyBlob_.size();
    if ((length > threshold) && (option.formatString != "l"))
    {
        length = threshold;
    }

    w.Write(
        "(alg = {0}, param = {1}, key = {2:l})",
        algObjId_,
        const_buffer(paramBlob_.data(), paramBlob_.size()),
        const_buffer(keyBlob_.data(), length));
}

#endif

X509Identity::Type X509PubKey::IdType() const
{
    return X509Identity::PublicKey;
}
