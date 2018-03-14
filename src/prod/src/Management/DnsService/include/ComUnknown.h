// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define K_BEGIN_COM_INTERFACE_LIST(TClass)                                               \
public:                                                                                  \
    STDMETHOD_(ULONG, AddRef)(void) { return KShared::AddRef(); }   \
    STDMETHOD_(ULONG, Release)(void) { return KShared::Release();} \
    STDMETHOD(QueryInterface)(REFIID riid, void** ppResult)                     \
    {                                                                                    \
    if (ppResult == nullptr) { return E_POINTER; }

#define K_COM_INTERFACE_ITEM(iid, TInterface) \
    if (riid == (iid)) { *ppResult = static_cast<TInterface*>(this); } else

#define K_END_COM_INTERFACE_LIST()                                                       \
        { *ppResult = nullptr; }                                                         \
        \
        if (*ppResult != nullptr)                                                        \
        {                                                                                \
        KShared::AddRef();                                                           \
        return S_OK;                                                                     \
        }                                                                                \
        else                                                                             \
        {                                                                                \
        return E_NOINTERFACE;                                                            \
        }                                                                                \
    }
