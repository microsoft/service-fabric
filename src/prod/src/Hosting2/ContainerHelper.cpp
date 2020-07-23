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

ErrorCode ContainerHelper::GetContainerLogRoot(wstring & containerLogRoot)
{
    wstring fabricLogRoot;
    auto error = Common::FabricEnvironment::GetFabricLogRoot(fabricLogRoot);
    if (!error.IsSuccess())
    {
        TraceError(
            TraceTaskCodes::Hosting,
            TraceType_ContainerHelper,
            "GetFabricLogRoot(fabricLogRoot) failed. Container folder will not be marked.");
        return error;
    }

    containerLogRoot = Path::Combine(fabricLogRoot, L"Containers");
    if (!Directory::Exists(containerLogRoot))
    {
        TraceWarning(
            TraceTaskCodes::Hosting,
            TraceType_ContainerHelper,
            "Directory {0} does not exist. Container folder will not be marked.",
            containerLogRoot);
        return error;
    }

    return error;
}

void ContainerHelper::MarkContainerLogFolder(
    wstring const & appServiceId,
    bool forProcessing,
    bool markAllDirectories)
{
    /*
    Whenever a container crashes/fails to start, we create a marker file(ContainerLogRemovalMarkerFile) for FabricDCA to consume.
    FabricDCA will then delete the directory.
    If the flag "forProcessing" is set to true. It will create the ContainerLogProcessMarkerFile which informs DCA that the host is Container
    host and has FabricRuntime in it. FabricDCA can now start monitoring this file for logs.
    */
    TraceInfo(
        TraceTaskCodes::Hosting,
        TraceType_ContainerHelper,
        "MarkContainerLog for appServiceId:{0}, forProcessing:{1}, markAllDirectories:{2}",
        appServiceId,
        forProcessing,
        markAllDirectories);

    wstring containerLogRoot;
    auto error = GetContainerLogRoot(containerLogRoot);
    if (!error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Hosting,
            TraceType_ContainerHelper,
            "Container Log Root not found.");
        return;
    }

    vector<wstring> appServiceLogRoot;
    if (markAllDirectories)
    {
        appServiceLogRoot = Directory::GetSubDirectories(containerLogRoot, L"*.*", true, true);
    }
    else
    {
        appServiceLogRoot = Directory::GetSubDirectories(containerLogRoot, wformatString("*{0}*", appServiceId), true, true);
        TESTASSERT_IF(appServiceLogRoot.size() > 1, "Multiple log folders found for same appServiceId");
    }

    auto logDirectoriesCount = appServiceLogRoot.size();

    if (logDirectoriesCount  == 0)
    {
        TraceWarning(
            TraceTaskCodes::Hosting,
            TraceType_ContainerHelper,
            "Container Log Root not found at:{0} for appServiceId:{1}. Container folder will not be marked.",
            containerLogRoot,
            appServiceId);
        return;
    }

    TraceInfo(
        TraceTaskCodes::Hosting,
        TraceType_ContainerHelper,
        "MarkContainerLog folder root:{0}, number of subdirectories to process :{1}",
        containerLogRoot,
        logDirectoriesCount);

    wstring markerFile = forProcessing ? Constants::ContainerLogProcessMarkerFile : Constants::ContainerLogRemovalMarkerFile;
    wstring markerFileText = forProcessing ? L"ProcessContainerLog = true" : L"RemoveContainerLog = true";

    for (auto const& logRoot : appServiceLogRoot)
    {
        auto markerFilePath = Path::Combine(logRoot, markerFile);
        if (!File::Exists(markerFilePath))
        {
            FileWriter containerLogMarkerFile;
            error = containerLogMarkerFile.TryOpen(markerFilePath);
            if (!error.IsSuccess())
            {
                TraceWarning(
                    TraceTaskCodes::Hosting,
                    TraceType_ContainerHelper,
                    "Error while opening file:{0}, error:{1}. Container folder will not be marked.",
                    markerFilePath,
                    ::GetLastError());
            }
            else
            {
                containerLogMarkerFile.WriteUnicodeBuffer(markerFileText.c_str(), markerFileText.size());
                containerLogMarkerFile.Close();
            }
        }
    }
}