// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "windows.h"

#define MAX_VALUE_NAME 16383

using namespace std;
using namespace Common;

RegistryKey::RegistryKey(wstring const & name, bool const readOnly, bool const openExisting)
{
    DWORD isNew;
    if (openExisting)
    {
        error_ = RegOpenKeyEx(HKEY_LOCAL_MACHINE, name.c_str(), 0, readOnly ? KEY_READ : KEY_ALL_ACCESS, &key_);
        isNew = REG_OPENED_EXISTING_KEY;
    }
    else
    {
        error_ = RegCreateKeyEx(HKEY_LOCAL_MACHINE, name.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, readOnly? KEY_READ : KEY_ALL_ACCESS, NULL, &key_, &isNew);
    }
    initialized_ = error_ == ERROR_SUCCESS;
    existed_ = isNew == REG_OPENED_EXISTING_KEY;
    keyName_ = name;
}

RegistryKey::RegistryKey(wstring const & name, LPCWSTR machineName, bool const readOnly, bool const openExisting)
{
    error_ = RegConnectRegistry(machineName, HKEY_LOCAL_MACHINE, &key_);
    DWORD isNew;
    if (openExisting)
    {
        error_ = RegOpenKeyEx(key_, name.c_str(), 0, readOnly ? KEY_READ : KEY_ALL_ACCESS, &key_);
        isNew = REG_OPENED_EXISTING_KEY;
    }
    else
    {
        error_ = RegCreateKeyEx(key_, name.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, readOnly ? KEY_READ : KEY_ALL_ACCESS, NULL, &key_, &isNew);
    }

    initialized_ = error_ == ERROR_SUCCESS;
    existed_ = isNew == REG_OPENED_EXISTING_KEY;
    keyName_ = name;
}

bool RegistryKey::SetValue(wstring const & name, DWORD value)
{
    error_ = RegSetValueEx(key_, name.c_str(), 0, REG_DWORD_LITTLE_ENDIAN, (const BYTE *) &value, sizeof(DWORD));
    return  IsValid;
}

bool RegistryKey::SetValue(wstring const & name, wstring const & value, bool typeRegSZ)
{
    error_ = RegSetValueEx(key_, name.c_str(), 0, typeRegSZ ? REG_SZ : REG_EXPAND_SZ, (const BYTE *) value.c_str(), (DWORD) value.size() * sizeof(wchar_t));
    return  IsValid;
}

bool RegistryKey::SetValue(wstring const & name, const BYTE * value, ULONG valueLen)
{
    error_ = RegSetValueEx(key_, name.c_str(), 0, REG_BINARY, value, valueLen);
    return  IsValid;
}

bool RegistryKey::SetValue(wstring const & name, vector<wstring> const & value)
{
    size_t size = 0;
    for (auto const & item : value)
    {
        size += item.size() + 1;
    }
    size++;

    vector<wchar_t> registryValue(size);
    size_t index = 0;
    for (auto const & item : value)
    {
        KMemCpySafe(&registryValue[index], (registryValue.size() - index) * sizeof(wchar_t), item.c_str(), item.size() * sizeof(wchar_t));
        index += item.size();
        registryValue[index++] = L'\0';
    }

    registryValue[index++] = L'\0';

    error_ = RegSetValueEx(key_, name.c_str(), 0, REG_MULTI_SZ, (const BYTE *) registryValue.data(), (DWORD) registryValue.size() * sizeof(wchar_t));
    return IsValid;
}

bool RegistryKey::GetValue(wstring const & name, DWORD & value)
{
    DWORD size = sizeof(DWORD);
    error_ = RegGetValue(key_, NULL, name.c_str(), RRF_RT_REG_DWORD, NULL, &value, &size);
    return  IsValid;
}

bool RegistryKey::GetValue(wstring const & name, wstring & value)
{
    return GetValue(name, value, false);
}

bool RegistryKey::GetValue(wstring const & name, wstring & value, bool expandEnvironmentStrings)
{
    DWORD size = MAX_VALUE_NAME;
    wstring val;
    val.resize(static_cast<size_t>(size) / sizeof(wchar_t) + 1);
    DWORD type = REG_EXPAND_SZ;
    error_ = RegGetValue(key_, NULL, name.c_str(), RRF_NOEXPAND | RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_SZ, &type, reinterpret_cast<BYTE *>(&val[0]), &size);

    val.resize(size / sizeof(wchar_t) - 1);

    if (expandEnvironmentStrings)
    {
        wstring expandedVal;
        if (Environment::ExpandEnvironmentStrings(val, expandedVal))
        {
            val = move(expandedVal);
        }
        else
        {
            return false;
        }
    }

    value = val;
    return  IsValid;
}

bool RegistryKey::GetValue(wstring const & name, vector<wstring> & value)
{
    DWORD type, size;
    vector<wstring> target;
    error_ = RegQueryValueExW(
        key_,
        name.c_str(),
        NULL,
        &type,
        NULL,
        &size );
    if(!IsValid)
    {
        return false;
    }

    ASSERT_IF(type != REG_MULTI_SZ, "The registry value is expected to be of REG_MULTI_SZ type");

    vector<wchar_t> registryValue(size/sizeof(wchar_t));
    error_ = RegQueryValueExW(
        key_,
        name.c_str(),
        NULL,
        NULL,
        reinterpret_cast<LPBYTE>(registryValue.data()),
        &size);
    if(!IsValid)
    {
        return false;
    }

    vector<wstring> tempValue;
    size_t index = 0;       
    wstring item = wstring(&registryValue[index]);
    while (item.size() > 0)
    {
        tempValue.push_back(item);
        index += item.size() + 1;
        item = wstring(&registryValue[index]);
    }
   
    value = move(tempValue);

    return true;
}

bool RegistryKey::DeleteValue(wstring const & name)
{
    error_ = ::RegDeleteValue(key_, name.c_str());
    return IsValid || error_ == ERROR_FILE_NOT_FOUND;
}

bool RegistryKey::DeleteKey()
{
    error_ = ::RegDeleteKeyEx(HKEY_LOCAL_MACHINE, keyName_.c_str(), KEY_WOW64_64KEY, 0);
    return IsValid;
}

RegistryKey::~RegistryKey()
{
    if (initialized_)
    {
        ::RegCloseKey(key_);
    }
}
