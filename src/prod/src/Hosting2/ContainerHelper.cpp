// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Hosting2;
using namespace Common;
using namespace ServiceModel;

StringLiteral TraceType_ContainerHelper = "ContainerHelper";

ContainerHelper * ContainerHelper::singleton_;
INIT_ONCE ContainerHelper::initOnceContainerHelper_;

ContainerHelper::ContainerHelper():
    isInitialized_(false),
    osBuildNumber_(),
    lock_()
{
}

ContainerHelper::~ContainerHelper()
{}

BOOL CALLBACK ContainerHelper::InitContainerHelper(PINIT_ONCE, PVOID, PVOID *)
{
    singleton_ = new ContainerHelper();
    return TRUE;
}

ContainerHelper& ContainerHelper::GetContainerHelper()
{
    PVOID lpContext = NULL;
    BOOL  bStatus = FALSE;

    bStatus = ::InitOnceExecuteOnce(
        &ContainerHelper::initOnceContainerHelper_,
        ContainerHelper::InitContainerHelper,
        NULL,
        &lpContext);

    ASSERT_IF(!bStatus, "Failed to initialize ContainerHelper singleton");
    return *(ContainerHelper::singleton_);
}

ErrorCode ContainerHelper::GetContainerImageName(ImageOverridesDescription const& images, __inout std::wstring & imageName)
{
    ErrorCode error(ErrorCodeValue::Success);
    if (!isInitialized_)
    {
        AcquireWriteLock lock(lock_);

        //This is a double check here. This will prevent other threads from querying the OS build number.
        if (!isInitialized_)
        {
            error = GetCurrentOsBuildNumber(osBuildNumber_);
            if (!error.IsSuccess())
            {
                TraceError(TraceTaskCodes::Hosting, TraceType_ContainerHelper, "Unable to determine current OS Build Number {0}", error);
                return error;
            }

            isInitialized_ = true;
        }
    }

    for (auto const& image : images.Images)
    {
        //Find the image name in overrides which matched the corresponding build number for OS.
        if(StringUtility::AreEqualCaseInsensitive(image.Os, osBuildNumber_))
        {
            imageName = image.Name;
            TraceInfo(TraceTaskCodes::Hosting, TraceType_ContainerHelper, "Found Image {0} matching Build Number:{1}", imageName, image.Os);

            break;
        }
        else if (image.Os.empty())
        {
            // A default image exist with no Os tag and we should use that.
            imageName = image.Name;
            TraceInfo(TraceTaskCodes::Hosting, TraceType_ContainerHelper, "Default Image {0} present", imageName);
        }
    }

    return error;
}

ErrorCode ContainerHelper::GetCurrentOsBuildNumber(__out wstring & os)
{
#ifdef PLATFORM_UNIX
    os = L"";
    return ErrorCodeValue::Success;
#else
    typedef LONG(WINAPI* rtlGetVersion)(RTL_OSVERSIONINFOW*);

    RTL_OSVERSIONINFOW pk_OsVer;

    pk_OsVer.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

    HMODULE h_NtDll = GetModuleHandleW(L"ntdll");
    rtlGetVersion f_RtlGetVersion = (rtlGetVersion)GetProcAddress(h_NtDll, "RtlGetVersion");

    if (!f_RtlGetVersion)
    {
        TraceError(TraceTaskCodes::Hosting, TraceType_ContainerHelper, "Unable to load ntdll.dll");

        return ErrorCode(ErrorCodeValue::InvalidOperation, wformatString(StringResource::Get(IDS_HOSTING_DllLoadFailed), L"ntdll.dll"));
    }

    auto status = f_RtlGetVersion(&pk_OsVer);

    if (status == 0) //Success
    {
        os = StringUtility::ToWString<DWORD>(pk_OsVer.dwBuildNumber);

        TraceInfo(TraceTaskCodes::Hosting, TraceType_ContainerHelper, "GetCurrentOsBuildNumber - Build Number {0}", os);

        return ErrorCodeValue::Success;
    }

    return ErrorCode(ErrorCodeValue::InvalidOperation, wformatString(StringResource::Get(IDS_HOSTING_RtlGetVersionFailed), status));
#endif
}
