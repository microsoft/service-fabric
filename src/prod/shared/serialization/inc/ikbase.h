// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------




#pragma once

__interface IKBase
{
    //
    //
    // Return values:
    //    STATUS_SUCCESS
    //    STATUS_INSUFFICIENT_RESOURCES
    //    STATUS_NOINTERFACE
    //
    NTSTATUS QueryInterface(
        __in  GUID&  InterfaceId,
        __out VOID **Interface
        );


    LONG AddRef();
    LONG Release();
};

//
// KUniquePtr IKBase Deleter Definition
//
template <class T>
struct IKBaseDeleter
{
    static void Delete(T * pointer)
    {
        if (pointer != nullptr)
        {
            pointer->Release();
        }
    }
};
