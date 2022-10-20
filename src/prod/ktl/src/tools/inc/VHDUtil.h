/*++

Copyright (c) 2012  Microsoft Corporation

Module Name:

    VHDUtil.h

Abstract:

    This file defines a simple utility class to create and delete VHDs for testing. 

Author:

    Peng Li (pengli)    24-June-2012

Environment:

    User Mode only

Notes:

Revision History:

--*/

#pragma once

class VHDUtil
{
public:

    static BOOLEAN
    CreateVHD(
        __in_z LPCWSTR VHDFilePath,
        __in ULONG SizeInMB,
        __in_z LPCWSTR VolumeLabel = L"",
        __in BOOLEAN KeepAttached = TRUE
        );

    static BOOLEAN
    DeleteVHD(
        __in_z LPCWSTR VHDFilePath
        );    

    static BOOLEAN
    AttachVHD(
        __in_z LPCWSTR VHDFilePath        
        );    

    static BOOLEAN
    DetachVHD(
        __in_z LPCWSTR VHDFilePath
        );        

    static DWORD 
    RunDiskPart(
        __in_z LPCWSTR ScriptFilePath
        );
};

//
// An ephemeral VHD disk for testing.
// A backing VHD is created and mounted when a VHDDisk object is constructed.
// The VHD disk is dismounted and its backing VHD file is deleted when the VHDDisk object is destructed.
//

class VHDDisk : public KObject<VHDDisk>, public KShared<VHDDisk>
{
    K_FORCE_SHARED(VHDDisk);

public:

    static NTSTATUS
    Create(
        __out VHDDisk::SPtr& Result,
        __in GUID& DiskId, 
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __in_z LPCWSTR VHDFileName,
        __in ULONG SizeInMB,
        __in_z LPCWSTR VolumeLabel = L""
        );

    GUID const& DiskId();
    LPCWSTR FilePath();

    BOOLEAN Attach();
    BOOLEAN Detach();

private:

    VHDDisk(
        __in GUID& DiskId,
        __in_z LPCWSTR VHDFilePath,
        __in ULONG SizeInMB,
        __in_z LPCWSTR VolumeLabel    
        );    

    const GUID _DiskId;
    KWString _FullyQualifiedFilePath;
    BOOLEAN _IsAttached;
};

