// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>

// Common.h need to be included before boost to avoid redefinition error between ntdef.h and winnt.h
#include "../../../../Common/Common.h"
#include "FabricRuntime_.h"
#include "FabricTypes_.h"

// This is workaround to resolve the macro redefinition warning C4005 and make sure the correct macro is being used from bldver.h
// One is come from Common.h--->bldver.h
// The other is come from boost--->windows sdk--->ntverp.h
#pragma push_macro("VER_PRODUCTBUILD")
#pragma push_macro("VER_PRODUCTBUILD_QFE")
#pragma push_macro("VER_PRODUCTMAJORVERSION")
#pragma push_macro("VER_PRODUCTMINORVERSION")
#pragma push_macro("VER_PRODUCTVERSION_W")
#pragma push_macro("VER_PRODUCTVERSION")
#pragma push_macro("VER_PRODUCTVERSION_STR")
#pragma push_macro("VER_COMPANYNAME_STR")
#pragma push_macro("VER_PRODUCTNAME_STR")
#undef VER_PRODUCTBUILD
#undef VER_PRODUCTBUILD_QFE
#undef VER_PRODUCTMAJORVERSION
#undef VER_PRODUCTMINORVERSION
#undef VER_PRODUCTVERSION_W
#undef VER_PRODUCTVERSION
#undef VER_PRODUCTVERSION_STR
#undef VER_COMPANYNAME_STR
#undef VER_PRODUCTNAME_STR

#include <boost/asio.hpp>
#include <boost/thread.hpp>

#pragma pop_macro("VER_PRODUCTBUILD")
#pragma pop_macro("VER_PRODUCTBUILD_QFE")
#pragma pop_macro("VER_PRODUCTMAJORVERSION")
#pragma pop_macro("VER_PRODUCTMINORVERSION")
#pragma pop_macro("VER_PRODUCTVERSION_W")
#pragma pop_macro("VER_PRODUCTVERSION")
#pragma pop_macro("VER_PRODUCTVERSION_STR")
#pragma pop_macro("VER_COMPANYNAME_STR")
#pragma pop_macro("VER_PRODUCTNAME_STR")

#ifdef PLATFORM_UNIX
#include <sal.h>
#include "PAL.h"
#define _ASSERT(x) ASSERT(x)
typedef void *HMODULE;
#endif
// We need to use whatever includes windows.h after boost - otherwise boost isn't too happy about winsock
// is already included. 
#include "../dependencies/inc/reliable_collections.h"

#if defined(PLATFORM_UNIX)
// On Linux, this is used to correlate kernel and user space transactions
#define BLOCKSTORE_SERVICE_TRACKING_BYTES 8
#else
// Windows doesn't have support for IO tracking IDs currently
#define BLOCKSTORE_SERVICE_TRACKING_BYTES 0
#endif

// Define the constants used across the service implementation
namespace BlockStore
{
    namespace Constants
    {
        // Disk block size
        constexpr uint32_t SizeDiskBlock = 16384;

        // Size of buffer for disk management request
        constexpr uint32_t SizeDiskManagementRequestBuffer = 16384;

        // Maximum length (in bytes) supported for the LUN ID
        constexpr uint16_t MaxLengthLUNID = 50 * sizeof(wchar_t);

        // Size of the payload struct sent by the driver
        constexpr uint16_t SizeDriverPayloadStruct = 48 + BLOCKSTORE_SERVICE_TRACKING_BYTES;

        // Marker indicating end of payload
        constexpr uint32_t MarkerEndOfPayload = 0xd0d0d0d0;

        // Size of the EndOfPayload marker
        constexpr uint16_t SizeMarkerEndOfPayload = sizeof(MarkerEndOfPayload);

        // Size of the payload header exchanged between the driver and the service
        constexpr uint16_t SizeBlockStoreRequestPayloadHeader = SizeDriverPayloadStruct + MaxLengthLUNID + SizeMarkerEndOfPayload;

        // Maximum number of LUN registrations supported by the service.
        //
        // TODO: Revisit this to make it dynamic.
        constexpr uint16_t MaxLUNRegistrations = 100;

        // Default port of 0 will result in random port being assigned.
        constexpr uint32_t DefaultServicePort = 0;

        // Default timeout for transaction in seconds
        constexpr seconds DefaultTransactionTimeoutSeconds = 4s;

        // Constant to be used for 32bit padding.
        constexpr uint32_t Padding = 0xdeadbeef;

        //Format: {0}_{1}_{2}, {DiskSize}_{DiskSizeUnit}_{Mounted}
        constexpr uint8_t RegistrationInfoFormat = 3;

        // Maximum number of update retry
        constexpr uint8_t MaxUpdateRetry = 10;

        // Default HTTP version 1.1
        constexpr uint8_t DefaultHTTPVersion = 11;

        // Default Block Store Service Name
#ifdef PLATFORM_UNIX
        constexpr char16_t *DefaultBlockStoreServiceName = (char16_t *)u"SFBlockStoreService";
        constexpr wchar_t DockerPluginFilenameLinux[] = L"VolumeDriver/sfvolumedisk.json";
        constexpr uint8_t DeactivationEventFileRetryCount = 100;
        constexpr uint32_t DeactivationEventFileCreateTimeout = 2;
#else
        constexpr char16_t *DefaultBlockStoreServiceName = u"SFBlockStoreService";
#endif
        // Default Docker Volume Driver Register command successful return value
        constexpr char DockerRegisterSuccessful[] = "RegisterSuccessful";

        // Maximum bytes read in json file
        constexpr uint8_t MaxBytesRead = 100;

        // Default wait time for ReliableCollection get ready in seconds
        constexpr uint8_t DefaultWaitTimeForReliableCollectionSeconds = 15;

        // Number of times to retry registration with Volume Driver
        constexpr uint8_t VolumeDriverRegistrationRetryCount = 5;

        // Number of seconds to wait before retrying registration with Volume Driver
        constexpr uint32_t VolumeDriverRegistrationRetryIntervalInSeconds = 5;

        // Number of times to try waiting for Reliable Collection Services to get ready
        constexpr uint8_t RCSReadyRetryCount = 5;

        // Number of seconds to wait between tries for RCS to get ready
        constexpr uint32_t RCSReadyRetryIntervalInSeconds = 10;

        // Make sure DockerPluginName("sfvolumedisk") match the name defined in the following files:
        // src/prod/src/managed/Api/src/System/Fabric/Management/ImageBuilder/SingleInstance/VolumeParametersVolumeDisk.cs
        // src/prod/src/BlockStore/Service/Native/inc/common.h
        // src/prod/src/BlockStore/Service/VolumeDriver/SetupEntryPoint/Program.cs
        //
        // This is the path to the file containing port information for BlockStore Volume Driver.
        constexpr wchar_t DockerPluginFilename[] = L"\\SF\\VolumeDriver\\sfvolumedisk.json";

        constexpr char LocalHostIPV4Address[] = "127.0.0.1";

    }

    namespace Mode
    {
        enum Enum
        {
            Read = 1,
            Write,
            RegisterLU,
            UnregisterLU,
            FetchRegisteredLUList,
            UnmountLU,
            MountLU,
            OperationCompleted = 0x827,
            OperationFailed = 0xeee,
            InvalidPayload = 0xfee,
            ServerInvalidPayload = 0xfef
        };
    }
}

// Define the type used to represent the data bytes
// exchanged between the BlockStore service and the client. 
typedef unsigned char BlockStoreByte;

//
// Specialized serializer for vector<BlockStoreByte> 
//
namespace service_fabric
{
    template <>
    struct reliable_value_serializer_traits<vector<BlockStoreByte>>
    {
        using value_holder = const BlockStoreByte *;

        static const_slice serialize(const vector<BlockStoreByte> &obj, value_holder &holder)
        {
            UNREFERENCED_PARAMETER(holder);
            // Casting here because const_slice is designed to be bytes buffer
            return { (char *)obj.data(), obj.size() };
        }

        static void deserialize(vector<BlockStoreByte> &obj, const void *bytes, size_t len)
        {
            // @PERF - If we can guarantee the lifetime of returned buffer, we should be able to pass 
            // pointer directly 
            obj.assign((const BlockStoreByte *)bytes, (const BlockStoreByte *)bytes + len);
        }
    };
}
