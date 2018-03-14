// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace FabricTest;
using namespace Common;
using namespace Federation;
using namespace Reliability;
using namespace std;
using namespace Naming;

const StringLiteral TraceSource("FabricTest.NamingState");

void FabricTestNamingState::AddPropertyToMap(
    NamingUri const & namingName, 
    std::wstring const & propertyName, 
    ::FABRIC_PROPERTY_TYPE_ID propertyType, 
    std::vector<byte> const & bytes,
    std::wstring const & customTypeId,
    __in PropertyStateMap & propertyStateMap)
{
    AcquireWriteLock lock(lock_);

    auto iter = propertyStateMap.find(namingName);
    if (iter == propertyStateMap.end())
    {
        propertyStateMap[namingName] = NamedPropertyMap();
    }

    NamedPropertyData data = { propertyName, propertyType, bytes, customTypeId };
    propertyStateMap[namingName][propertyName] = move(data);
}

void FabricTestNamingState::AddProperty(
    NamingUri const & namingName, 
    std::wstring const & propertyName, 
    ::FABRIC_PROPERTY_TYPE_ID propertyType, 
    std::vector<byte> const & bytes,
    std::wstring const & customTypeId)
{
    AddPropertyToMap(
        namingName,
        propertyName,
        propertyType,
        bytes,
        customTypeId,
        propertyState_);
}

void FabricTestNamingState::AddProperties(
    NamingUri const & namingName, 
    vector<NamedPropertyData> const & properties)
{
    AcquireWriteLock lock(lock_);
    auto iter = propertyState_.find(namingName);
    if (iter == propertyState_.end())
    {
        propertyState_[namingName] = NamedPropertyMap();
    }

    for (auto it = properties.begin(); it != properties.end(); ++it)
    {
        propertyState_[namingName][it->PropertyName] = *it;
    }
}

void FabricTestNamingState::RemoveProperty(
    Common::NamingUri const & namingName, 
    std::wstring const & propertyName)
{
    AcquireWriteLock lock(lock_);

    auto iter = propertyState_.find(namingName);
    if (iter != propertyState_.end())
    {
        NamedPropertyMap & propertyMap = iter->second;

        auto iter2 = propertyMap.find(propertyName);
        if (iter2 != propertyMap.end())
        {
            propertyMap.erase(iter2);
        }
    }
}

void FabricTestNamingState::AddPropertyEnumerationResult(
    NamingUri const & namingName, 
    std::wstring const & propertyName, 
    ::FABRIC_PROPERTY_TYPE_ID propertyType, 
    std::vector<byte> const & bytes,
    std::wstring const & customTypeId)
{
    AddPropertyToMap(
        namingName,
        propertyName,
        propertyType,
        bytes,
        customTypeId,
        propertyEnumerationResults_);
}

bool FabricTestNamingState::VerifyPropertyEnumerationResults(Common::NamingUri const & namingName, bool includeValues)
{
    AcquireReadLock lock(lock_);

    auto namingIter = propertyState_.find(namingName);
    auto enumerationIter = propertyEnumerationResults_.find(namingName);

    if ((namingIter != propertyState_.end() && enumerationIter == propertyEnumerationResults_.end()) ||
        (namingIter == propertyState_.end() && enumerationIter != propertyEnumerationResults_.end()))
    {
        TestSession::WriteWarning(
            TraceSource, 
            "Enumeration verification failed: naming = {0} enumerated = {1}",
            namingIter == propertyState_.end(),
            enumerationIter == propertyEnumerationResults_.end());

        return false;
    }

    if (namingIter == propertyState_.end())
    {
        return true;
    }

    NamedPropertyMap const & namingProperties = namingIter->second;
    NamedPropertyMap const & enumeratedProperties = enumerationIter->second;

    if (namingProperties.size() != enumeratedProperties.size())
    {
        TestSession::WriteWarning(
            TraceSource, 
            "Enumeration Verification failed: naming = {0} enumerated = {1}", 
            namingProperties.size(),
            enumeratedProperties.size());
        return false;
    }

    for (auto iter = namingProperties.begin(); iter != namingProperties.end(); ++iter)
    {
        wstring const & propertyName = iter->first;
        auto findIter = enumeratedProperties.find(propertyName);

        if (findIter == enumeratedProperties.end())
        {
            TestSession::WriteWarning(
                TraceSource, 
                "Enumeration Verification failed: missing enumerated property = {0}",
                propertyName);

            return false;
        }

        NamedPropertyData const & namingData = iter->second;
        NamedPropertyData const & enumeratedData = findIter->second;

        if (!AreEqual(namingData, enumeratedData, includeValues))
        {
            TestSession::WriteWarning(
                TraceSource, 
                "Enumeration Verification failed: naming = {0} enumerated = {1}",
                ToString(namingData),
                ToString(enumeratedData));

            return false;
        }
    }

    return true;
}

void FabricTestNamingState::SetPropertyEnumerationToken(
    Common::NamingUri const & namingName, 
    ComPointer<IFabricPropertyEnumerationResult> const & enumerationToken)
{
    AcquireWriteLock lock(lock_);

    propertyEnumerationTokens_[namingName] = enumerationToken;
}

Common::ComPointer<IFabricPropertyEnumerationResult> FabricTestNamingState::GetPropertyEnumerationToken(Common::NamingUri const & namingName)
{
    AcquireReadLock lock(lock_);

    auto iter = propertyEnumerationTokens_.find(namingName);
    if (iter != propertyEnumerationTokens_.end())
    {
        return iter->second;
    }
    else
    {
        return ComPointer<IFabricPropertyEnumerationResult>();
    }
}

void FabricTestNamingState::ClearPropertyEnumerationResults(Common::NamingUri const & namingName)
{
    AcquireWriteLock lock(lock_);

    propertyEnumerationResults_.erase(namingName);
    propertyEnumerationTokens_.erase(namingName);
}

void FabricTestNamingState::SetNameEnumerationToken(Common::NamingUri const & parent, ComPointer<IFabricNameEnumerationResult> const & enumResult)
{
    AcquireWriteLock lock(lock_);

    nameEnumerationTokens_[parent] = enumResult;
}

Common::ComPointer<IFabricNameEnumerationResult> FabricTestNamingState::GetNameEnumerationToken(Common::NamingUri const & parent)
{
    AcquireWriteLock lock(lock_);

    auto iter = nameEnumerationTokens_.find(parent);
    if (iter != nameEnumerationTokens_.end())
    {
        return iter->second;
    }
    else
    {
        return ComPointer<IFabricNameEnumerationResult>();
    }
}

void FabricTestNamingState::ClearNameEnumerationResults(Common::NamingUri const & parent)
{
    AcquireWriteLock lock(lock_);

    nameEnumerationTokens_.erase(parent);
}

std::wstring FabricTestNamingState::ToString(NamedPropertyData const & data)
{
    wstring value;
    StringWriter writer(value);
    writer.Write("NamedPropertyData[{0}, {1}, {2}, {3}]", data.PropertyName, static_cast<int>(data.PropertyType), data.Bytes.size(), data.CustomTypeId);

    return value;
}

bool FabricTestNamingState::AreEqual(NamedPropertyData const & left, NamedPropertyData const & right, bool includeValues)
{
    bool isMatch = 
        left.PropertyName == right.PropertyName &&
        left.PropertyType == right.PropertyType &&
        left.CustomTypeId == right.CustomTypeId;

    if (isMatch && includeValues)
    {
        isMatch = (left.Bytes.size() == right.Bytes.size());

        for (size_t ix = 0; ix < left.Bytes.size() && isMatch; ++ix)
        {
            if (left.Bytes[ix] != right.Bytes[ix])
            {
                TestSession::WriteWarning(
                    TraceSource, 
                    "Enumeration Verification failed: byte index = {0} naming = {1} enumerated = {2}",
                    ix,
                    left.Bytes[ix],
                    right.Bytes[ix]);

                isMatch = false;
            }
        }
    }

    return isMatch;
}
