// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef _METADATA_EXPORTS_
#define _METADATA_EXPORTS_



// General creation function for ClassFactory semantics.
STDAPI  MetaDataDllGetClassObject(REFCLSID rclsid, REFIID riid, void ** ppv);

// Specific creation function to get IMetaDataDispenser(Ex) interface.
HRESULT InternalCreateMetaDataDispenser(REFIID riid, void ** pMetaDataDispenserOut);

STDAPI  GetMDInternalInterface(
    LPVOID      pData, 
    ULONG       cbData, 
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnk);              // [out] Return interface on success.

STDAPI GetMDInternalInterfaceFromPublic(
    IUnknown    *pIUnkPublic,           // [IN] Given scope.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnkInternal);      // [out] Return interface on success.

STDAPI GetMDPublicInterfaceFromInternal(
    void        *pIUnkPublic,           // [IN] Given scope.
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnkInternal);      // [out] Return interface on success.

STDAPI MDReOpenMetaDataWithMemory(
    void        *pImport,               // [IN] Given scope. public interfaces
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData);                // [in] Size of the data pointed to by pData.

STDAPI MDReOpenMetaDataWithMemoryEx(
    void        *pImport,               // [IN] Given scope. public interfaces
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData,                 // [in] Size of the data pointed to by pData.
    DWORD       dwReOpenFlags);         // [in] ReOpen flags      



#endif // _METADATA_EXPORTS_
