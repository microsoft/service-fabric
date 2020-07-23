// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifndef __IldbSymLib_h__
#define __IldbSymLib_h__

// Get the IClassFactory for one of the ILDB symbols CLSIDs
STDAPI IldbSymbolsGetClassObject(REFCLSID rclsid, REFIID riid, void** ppvObject);

// Create an inststance of an ILDB ISymUnmanagedReader, ISymUnmanagedWriter or ISymUnmanagedBinder
STDAPI IldbSymbolsCreateInstance(REFCLSID rclsid, REFIID riid, void** ppvIUnknown);

#endif // __IldbSymLib_h__
