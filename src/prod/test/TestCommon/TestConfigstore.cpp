// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TestCommon;

TestConfigStore::TestConfigStore(Common::ConfigStoreSPtr const & store)
    :   store_(store),
        overrides_(ConfigSettings()),
        lock_()
{
    sink_.Register(*this);
}

wstring TestConfigStore::ReadString(wstring const & section, wstring const & key, __out bool & isEncrypted) const
{
    AcquireReadLock lock(lock_);

    wstring result = overrides_.ReadString(section, key, isEncrypted);
    if (result.size() > 0)
    {
        return result;
    }

    if (resetSections_.find(section) == resetSections_.end())
    {
        return store_->ReadString(section, key, isEncrypted);
    }

    return wstring();
}

void TestConfigStore::GetSections(StringCollection & sectionNames, wstring const & partialName) const
{
    AcquireReadLock lock(lock_);

    store_->GetSections(sectionNames, partialName);

    StringCollection overrideSections;
    overrides_.GetSections(overrideSections, partialName);

    for (auto it = overrideSections.begin(); it != overrideSections.end(); ++it)
    {
        if (find(sectionNames.begin(), sectionNames.end(), *it) == sectionNames.end())
        {
            sectionNames.push_back(*it);
        }
    }
}

void TestConfigStore::GetKeys(wstring const & sectionName, StringCollection & keyNames, wstring const & partialName) const
{
    AcquireReadLock lock(lock_);

    if (resetSections_.find(sectionName) == resetSections_.end())
    {
        store_->GetKeys(sectionName, keyNames, partialName);
    }

    StringCollection overrideKeys;
    overrides_.GetKeys(sectionName, overrideKeys, partialName);

    for (auto it = overrideKeys.begin(); it != overrideKeys.end(); ++it)
    {
        bool isEncrypted;
        wstring value = overrides_.ReadString(sectionName, *it, isEncrypted);

        auto it2 = find(keyNames.begin(), keyNames.end(), *it);
        if (value.size() > 0)
        {
            if (it2 == keyNames.end())
            {
                keyNames.push_back(*it);
            }
        }
        else
        {
            if (it2 != keyNames.end())
            {
                keyNames.erase(it2);
            }
        }
    }
}

bool TestConfigStore::UpdateConfig(StringCollection const & settings)
{
    return Set(settings, true);
}

bool TestConfigStore::SetConfig(StringCollection const & settings)
{
    return Set(settings, false);
}

bool TestConfigStore::RemoveSection(wstring const & section)
{
    overrides_.Remove(section);
    return true;
}

bool TestConfigStore::Set(StringCollection const & settings, bool update)
{
    vector<pair<wstring, wstring>> updates;

    {
        AcquireWriteLock lock(lock_);

        for (auto it = settings.begin(); it != settings.end(); ++it)
        {
            size_t index1 = it->find_first_of(L'=');
            if (index1 == wstring::npos)
            {
                Assert::CodingError("Invalid input: {0}", *it);
            }

            ConfigParameter setting;
            setting.Value = it->substr(index1 + 1);

            size_t index2 = it->find_first_of(L'.');
            if (index2 == wstring::npos || index2 > index1)
            {
                resetSections_.insert(it->substr(0, index1));
                continue;
            }

            wstring section = it->substr(0, index2);
            setting.Name = it->substr(index2 + 1, index1 - index2 - 1);

            StringUtility::TrimSpaces(setting.Name);
            StringUtility::TrimSpaces(setting.Value);
            StringUtility::TrimSpaces(section);

            if (section.size() == 0 || setting.Name.size() == 0)
            {
                Assert::CodingError("Invalid input: {0}", *it);
            }

            if(update)
            {
                updates.push_back(make_pair(section, setting.Name));
            }

            overrides_.Set(section, move(setting));
        }
    }

    bool result = true;
    for (auto it = updates.begin(); it != updates.end(); ++it)
    {
        result = result && OnUpdate(it->first, it->second);
    }

    return result;
}
