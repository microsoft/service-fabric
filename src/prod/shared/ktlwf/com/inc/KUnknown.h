// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define K_BEGIN_COM_INTERFACE_LIST(TClass)                                               \
public:                                                                                  \
    STDMETHOD_(ULONG, AddRef)(void) { return (ULONG)KSharedBase::AddRef(); }   \
    STDMETHOD_(ULONG, Release)(void) { return (ULONG)KSharedBase::Release(); } \
    STDMETHOD(QueryInterface)(REFIID riid, void ** result)                     \
    {                                                                                    \
        if (!result) { return Common::ComUtility::OnPublicApiReturn(E_POINTER); }

#define K_END_COM_INTERFACE_LIST()                                                       \
        { *result = NULL; }                                                              \
                                                                                         \
        if (*result)                                                                     \
        {                                                                                \
            KSharedBase::AddRef();                                                       \
            return Common::ComUtility::OnPublicApiReturn(S_OK);                          \
        }                                                                                \
        else                                                                             \
        {                                                                                \
            return Common::ComUtility::OnPublicApiReturn(E_NOINTERFACE);                 \
        }                                                                                \
    }

#if DBG
#define K_I_SHARED() \
    public:\
        inline LONG IShared::AddRef() { return KSharedBase::AddRef(); } \
        inline LONG IShared::Release() { return KSharedBase::Release(); } \
        inline VOID IShared::AddTrackedSPtr(__inout PLIST_ENTRY SptrLinkage) { return KSharedBase::AddTrackedSPtr(SptrLinkage); } \
        inline VOID IShared::RemoveTrackedSPtr(__inout PLIST_ENTRY SptrLinkage) { return KSharedBase::RemoveTrackedSPtr(SptrLinkage); } \
    private:
#else
#define K_I_SHARED() \
    public:\
        inline LONG IShared::AddRef() { return KSharedBase::AddRef(); } \
        inline LONG IShared::Release() { return KSharedBase::Release(); } \
    private:
#endif

class IShared abstract
{
public:

    virtual LONG AddRef() = 0;

    virtual LONG Release() = 0;

#if DBG
    virtual VOID AddTrackedSPtr(__inout PLIST_ENTRY SptrLinkage) = 0;

    virtual VOID RemoveTrackedSPtr(__inout PLIST_ENTRY SptrLinkage) = 0;
#endif

};
