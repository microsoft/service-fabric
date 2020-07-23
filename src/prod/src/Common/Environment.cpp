// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <windns.h>
#include "Common/Environment.h"
#include "Common/Path.h"

#if defined(PLATFORM_UNIX)

#include <pwd.h>
#include <libgen.h>
#include <dlfcn.h>

extern char **environ;

#endif

namespace
{
    Common::Global<Common::RwLock> envLock = Common::make_global<Common::RwLock>();
}

namespace Common
{
    using namespace std;

    StringLiteral TraceType_Environment = "Environment";

    wstring const Environment::NewLine = L"\r\n";

    void Environment::GetEnvironmentVariable(wstring const & name, wstring& outValue, NOCLEAR)
    {
        size_t pos = outValue.size();
        ULONG valueLen = 16; // first shot
        outValue.resize(pos + valueLen);
        for(;;) 
        {
            ULONG n = ::GetEnvironmentVariableW(
                name.c_str(), &outValue[pos], valueLen);
            if (n == 0) 
            {
                THROW_SYSTEM_ERROR(microsoft::GetLastErrorCode(), "GetEnvironmentVariable");
            }

            size_t newSize;
            HRESULT hr = SizeTAdd(pos, n, &newSize);
            CODING_ERROR_ASSERT(SUCCEEDED(hr));

            if (n < valueLen)
            {
                outValue.resize(newSize);
                break;
            }
            else
            {
                valueLen = n;
                outValue.resize(newSize);
            }
        }
    }

    bool Environment::GetEnvironmentVariable(wstring const & name, wstring& outValue, NOTHROW)
    {
        int valueLen = 16; // first shot
        outValue.resize(valueLen);
        for(;;) 
        {
            int n = ::GetEnvironmentVariableW(
                name.c_str(), &outValue[0], valueLen);
            outValue.resize(n);
            if (n == 0) 
            {
                return false;
            }
            else if (n < valueLen)
            {
                break;
            }
            else
            {
                valueLen = n;
            }
        }
        return true;
    }

    void Environment::GetEnvironmentVariable(
        wstring const & name, wstring& outValue, wstring const & defaultValue)
    {
        if (!GetEnvironmentVariable(name, outValue, NOTHROW()))
        {
            outValue.assign( defaultValue );
        }
    }

    void Environment::GetEnvironmentVariable(wstring const & name, wstring& outValue)
    {
        outValue.clear();
        GetEnvironmentVariable(name, outValue, NOCLEAR());
    }

    wstring Environment::GetEnvironmentVariable ( wstring const & name )
    {
        wstring variable;
        GetEnvironmentVariable( name, variable, NOCLEAR() );
        return variable;
    }

    bool Environment::SetEnvironmentVariable(wstring const & name, wstring const& value)
    {
        BOOL result = ::SetEnvironmentVariableW( 
            name.c_str(), 
            value.empty() ? 0 : value.c_str());
        return result ? true : false;
    }

    void Environment::GetMachineName( wstring& buffer)
    {
        DWORD ccLen = DNS_MAX_LABEL_BUFFER_LENGTH;
        buffer.resize( ccLen );
        CHK_WBOOL( ::GetComputerNameExW(ComputerNameDnsHostname, &buffer[0], &ccLen) );
        buffer.resize( ccLen );
    }

    void Environment::GetMachineFQDN( wstring& buffer)
    {
        DWORD ccLen = DNS_MAX_NAME_BUFFER_LENGTH;
        buffer.resize( ccLen );
        CHK_WBOOL( ::GetComputerNameExW(ComputerNameDnsFullyQualified, &buffer[0], &ccLen) );
        buffer.resize( ccLen );
    }

    void Environment::GetUserName(wstring & buffer)
    {
        Environment::GetEnvironmentVariable( L"USERNAME", buffer );
    }

    void Environment::GetExecutablePath(wstring & buffer)
    {
        GetExecutableFileName(buffer);
        buffer = Path::GetDirectoryName(buffer);
    }

    wstring Environment::GetExecutablePath()
    {
        wstring path;
        GetExecutablePath(path);
        return path;
    }

    // Returns the name of the dll/exe that linked this Common.lib instance.
    void Environment::GetCurrentModuleFileName(wstring & buffer)
    {
        // See http://msdn.microsoft.com/en-us/library/ms683200(v=VS.85).aspx
        // The second parameter of GetModuleHandleEx can be a pointer to any
        // address mapped to the module.  We define a static to use as that
        // pointer.
        static int staticInThisModule = 0;

#if !defined(PLATFORM_UNIX)
        HMODULE currentModule = NULL;
        BOOL success = ::GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCTSTR>(&staticInThisModule),
            &currentModule);

        if (success == 0)
        {
            THROW(WinError(GetLastError()), "GetModuleHandleEx");
        }

        GetModuleFileName(currentModule, buffer);
#else
        int len = 0;
        char exeName[PATH_MAX] = {0};
        Dl_info dl_info;
        struct link_map *linkmap = 0;
        int found = dladdr1((void *) &staticInThisModule, &dl_info, (void **) &linkmap, RTLD_DL_LINKMAP);
        if (found && dl_info.dli_fname)
        {
            len = min(strlen(dl_info.dli_fname), sizeof(exeName) - 1);
            memcpy(exeName, dl_info.dli_fname, len + 1);
        }
        else
        {
            len = readlink("/proc/self/exe", exeName, sizeof(exeName) - 1);
        }
        if (len > 0)
        {
            exeName[len] = 0;
            buffer = StringUtility::Utf8ToUtf16(exeName);
        }
#endif
    }

    void Environment::GetExecutableFileName(wstring & buffer)
    {
        GetModuleFileName(NULL, buffer);
    }

    void Environment::GetModuleFileName(HMODULE module, wstring & buffer)
    {
        const int MaximumSaneStringLength = 10000; // if exceed this, something is terribly wrong
        const int InitialStringLength = 256;

        DWORD ccLen = InitialStringLength;

        for(;;)
        {
            buffer.resize(ccLen);
            DWORD ccCopied = ::GetModuleFileNameW(module, &buffer[0], ccLen);

            if (ccCopied == 0)
                THROW(WinError(GetLastError()), "GetModuleFileName");

            if (ccCopied < ccLen)
            {
                buffer.resize(ccCopied);
                return;
            }

            ccLen = ccLen * 2;

            if (ccLen > MaximumSaneStringLength)
            {
                throw bad_alloc();
            }	
        }
    }

    wstring Environment::GetExecutableFileName()
    {
        wstring result;
        GetExecutableFileName(result);
        return result;
    }

    // returns total available memory on the machine in bytes
    // for both Windows and Linux
    Common::ErrorCode Environment::GetAvailableMemoryInBytes(__out DWORDLONG& memoryInMB)
    {
#if defined(PLATFORM_UNIX)
        memoryInMB = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
        return ErrorCodeValue::Success;
#else
        MEMORYSTATUSEX memstatex;
        memstatex.dwLength = sizeof(memstatex);
        auto retValue = GlobalMemoryStatusEx(&memstatex);
        if (!retValue)
        {
            return ErrorCode::FromWin32Error();
        }
        else
        {
            memoryInMB = memstatex.ullTotalPhys;
            return ErrorCodeValue::Success;
        }
#endif
    }

    // returns mask of active processors
    // only for Windows
#if !defined(PLATFORM_UNIX)
    DWORD_PTR Environment::GetActiveProcessorsMask()
    {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return sysInfo.dwActiveProcessorMask;
    }
#endif

    // returns the number of processors
    // for both Windows and Linux
    DWORD Environment::GetNumberOfProcessors()
    {
#if defined(PLATFORM_UNIX)
        return sysconf(_SC_NPROCESSORS_ONLN);
#else
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        return sysInfo.dwNumberOfProcessors;
#endif
    }

    ErrorCode Environment::Expand(wstring const & str, __out wstring & expandedStr)
    {
        if (str.length() == 0) { return ErrorCode(ErrorCodeValue::Success); }

        DWORD length = ::ExpandEnvironmentStringsW(str.c_str(), NULL, 0);
        if (length == 0)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Environment,
                "ExpandEnvironmentStringsW failed {0}",
                error);
            return error;
        }
        vector<wchar_t> buffer(length);

        length = ::ExpandEnvironmentStringsW(str.c_str(), &(buffer.front()), length);
        if (length == 0)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Environment,
                "ExpandEnvironmentStringsW failed {0}",
                error);
            return error;
        }

        ASSERT_IFNOT(
            length == buffer.capacity(),  
            "{0}: length mismatch: expected = {1}, returned = {2}",
            __FUNCTION__,
            buffer.capacity(),
            length);

        expandedStr.assign(&(buffer.front()));
        return ErrorCode(ErrorCodeValue::Success);
    }

    wstring Environment::Expand(wstring const & str)
    {
        wstring expandedStr(L"");
        auto error = Environment::Expand(str, expandedStr);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_Environment,
                "Environment::Expand failed {0}",
                error);
        }

        return expandedStr;
    }

    bool Environment::ExpandEnvironmentStrings(wstring const & str, __out wstring & expandedStr)
    {
        return Environment::Expand(str, expandedStr).IsSuccess();
    }

#if !defined(PLATFORM_UNIX)
    ErrorCode Environment::GetEnvironmentMap(HANDLE userTokenHandle, bool inherit, __out EnvironmentMap & envMap)
    {
        LPVOID envBlock;
        if (!::CreateEnvironmentBlock(&envBlock, userTokenHandle, inherit ? TRUE : FALSE))
        {
            return ErrorCode::TraceReturn(
                ErrorCode::FromWin32Error(),
                TraceTaskCodes::Common,
                TraceType_Environment,
                "CreateEnvironmentBlock");
        }

        try
        {
            FromEnvironmentBlock(envBlock, envMap);

            if (!::DestroyEnvironmentBlock(envBlock))
            {
                TraceWarning(
                    TraceTaskCodes::Common,
                    TraceType_Environment,
                    "DestroyEnvironmentBlock failed. ErrorCode={0}",
                    ErrorCode::FromWin32Error());
            }
        }
        catch(...)
        {
            if (!::DestroyEnvironmentBlock(envBlock))
            {
                TraceWarning(
                    TraceTaskCodes::Common,
                    TraceType_Environment,
                    "DestroyEnvironmentBlock failed. ErrorCode={0}",
                    ErrorCode::FromWin32Error());
            }
            throw;
        }

        return ErrorCode(ErrorCodeValue::Success);
    }
#endif

    bool Environment::GetEnvironmentMap(__out EnvironmentMap & envMap)
    {
#if defined(PLATFORM_UNIX)
        char * env = *environ;
        int i = 1;
        
        Common::AcquireWriteLock grab(*envLock);
        
        while (env)
        {
            std::string temp(env);
            std::wstring tempW;
            StringUtility::Utf8ToUtf16(temp, tempW);
            size_t pos = tempW.find(L'=');
            if (pos != string::npos)
            {
                wstring key = tempW.substr(0, pos);
                wstring value = tempW.substr(pos + 1);
                envMap.insert(make_pair(key, value));
            }
            
            env = *(environ + i);
            i++;
        }
        
        return true;
#else
        LPWCH pEnvBlock = NULL;

        try
        {
            pEnvBlock = ::GetEnvironmentStringsW();
            if (pEnvBlock == NULL)
            {
                WriteWarning(
                    TraceType_Environment,
                    "GetEnvironmentStringsW failed {0}",
                    ::GetLastError());
                return false;
            }
            else
            {
                FromEnvironmentBlock(pEnvBlock, envMap);
                ::FreeEnvironmentStringsW(pEnvBlock);
            }
        }
        catch(...)
        {
            if (pEnvBlock)
            {
                ::FreeEnvironmentStringsW(pEnvBlock);
            }
            throw;
        }

        return true;
#endif
    }

    void Environment::FromEnvironmentBlock(
        LPVOID envBlock,
        __out EnvironmentMap & envMap)
    {
        envMap.clear();
        LPWSTR lpszVariable = (LPWSTR) envBlock;

        while (*lpszVariable)
        {
            // ignore the variables that starts with '='
            if (*lpszVariable == L'=')
            {
                lpszVariable += wcslen(lpszVariable) + 1;
                continue;
            }

            wstring variable(lpszVariable);
            size_t pos = variable.find(L'=');
            if (pos != string::npos)
            {
                wstring key = variable.substr(0, pos);
                wstring value = variable.substr(pos + 1);
                envMap.insert(make_pair(key, value));
            }

            lpszVariable += wcslen(lpszVariable) + 1;
        }
    }

    void Environment::ToEnvironmentBlock(
        Common::EnvironmentMap const & envMap,
        __out vector<wchar_t> & envBlock)
    {
        envBlock.resize(0);

        for(auto iter = envMap.begin(); iter != envMap.end(); iter++)
        {
            // push key
            for(auto it = iter->first.begin(); it != iter->first.end(); it++)
            {
                envBlock.push_back(*it);
            }

            // push =
            envBlock.push_back(L'=');

            // push value
            for(auto it = iter->second.begin(); it != iter->second.end(); it++)
            {
                envBlock.push_back(*it);
            }

            // null char
            envBlock.push_back(L'\0');
        }

        envBlock.push_back(L'\0');
    }

    wstring Environment::ToString(EnvironmentMap const & envMap)
    {
        wstring result;
        result.append(L"{");
        for(auto iter = envMap.cbegin(); iter != envMap.cend(); ++iter)
        {
            result.append(L"{");
            result.append(iter->first);
            result.append(L"=");
            result.append(iter->second);
            result.append(L"},");
        }
        result.append(L"}");

        return result;
    }

    ErrorCode Environment::GetModuleFileName2(HMODULE module, __out wstring & moduleFileName)
    {
        moduleFileName.resize(MAX_PATH);
        auto length = ::GetModuleFileName(module, &moduleFileName[0], MAX_PATH);
        if (length == 0)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Environment,
                "GetModuleFileName failed {0}",
                error);
            return error;
        }

        moduleFileName.resize(length);
        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode Environment::GetCurrentModuleFileName2(__out wstring & moduleFileName)
    {
        static int staticInThisModule2 = 0;

        HMODULE currentModule = NULL;
#if !defined(PLATFORM_UNIX)
        BOOL success = ::GetModuleHandleEx(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCTSTR>(&staticInThisModule2),
            &currentModule);

        if (success == 0)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Environment,
                "GetModuleHandleEx failed {0}",
                error);
            return error;
        }
#endif

        return Environment::GetModuleFileName2(currentModule, moduleFileName);
    }

#if !defined(PLATFORM_UNIX)
    ErrorCode Environment::GetFileVersion2(__in wstring const & fileName, __out wstring & fileVersion)
    {
        auto dwSize = ::GetFileVersionInfoSize(fileName.c_str(), NULL);
        if (dwSize == 0)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Environment,
                "GetFileVersionInfoSize failed {0}",
                error);
            return error;
        }

        vector<BYTE> buffer;
        buffer.resize(dwSize);
    
        auto success = ::GetFileVersionInfo(fileName.c_str(), 0, dwSize, (LPVOID)buffer.data());
        if (!success)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Environment,
                "GetFileVersionInfo failed {0}",
                error);
            return error;
        }

        VS_FIXEDFILEINFO * verInfo;
        UINT verInfoLen;
        success = ::VerQueryValue(buffer.data(), TEXT("\\"), (LPVOID*)&verInfo, &verInfoLen);
        if (!success)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Environment,
                "VerQueryValue failed {0}",
                error);
            return error;
        }
   
        int major = HIWORD(verInfo->dwProductVersionMS);
        int minor = LOWORD(verInfo->dwProductVersionMS);
        int revision = HIWORD(verInfo->dwProductVersionLS);
        int build = LOWORD(verInfo->dwProductVersionLS);

        fileVersion = wformatString("{0}.{1}.{2}.{3}", major, minor, revision, build);
        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode Environment::GetCurrentModuleFileVersion2(__out wstring & fileVersion)
    {
        wstring currentModuleName;
        auto error = Environment::GetCurrentModuleFileName2(currentModuleName);
        if (!error.IsSuccess()) { return error; }

        return Environment::GetFileVersion2(currentModuleName, fileVersion);
    }
#endif

    EnvironmentVariable & EnvironmentVariable::operator = (const std::wstring & newValue)
    {
#if defined(PLATFORM_UNIX)
        std::string tempKey;
        StringUtility::Utf16ToUtf8(name, tempKey);
        std::string tempVal;
        StringUtility::Utf16ToUtf8(newValue, tempVal);
        Common::AcquireWriteLock grab(*envLock);
        setenv(tempKey.c_str(), tempVal.empty() ? 0 : tempVal.c_str(), 1);
#else
        ::SetEnvironmentVariableW( 
            name.c_str(), 
            newValue.empty() ? 0 : newValue.c_str());
#endif
        currentValue = newValue;

        return *this;
    } // op =


    //
    // Adds the new path value at the end of the current path value
    //
    wstring EnvironmentVariable::AppendDirectoryToPathEnvVarValue(wstring const& currentPathValue, wstring const& pathValueToBeAdded)
    {
#ifndef PLATFORM_UNIX
        return wformatString(L"{0};{1}", currentPathValue, pathValueToBeAdded);
#else
        return wformatString(L"{0}:{1}", currentPathValue, pathValueToBeAdded);
#endif
    }

    DateTime Environment::GetLastRebootTime()
    {
#if defined(PLATFORM_UNIX)
        Assert::CodingError("GetLastRebootTime not implemented for Linux");
#else
        return DateTime::Now() - TimeSpan::FromMilliseconds((double)GetTickCount64());
#endif
    }

#ifdef PLATFORM_UNIX

    string Environment::GetHomeFolder()
    {
        auto user = getpwuid(geteuid());
        return string(user->pw_dir);
    }

    wstring Environment::GetHomeFolderW()
    {
        auto home = GetHomeFolder();
        wstring homew;
        StringUtility::Utf8ToUtf16(home, homew);
        return homew;
    }

    std::wstring Environment::GetObjectFolder()
    {
        auto objFolder = CommonConfig::GetConfig().ObjectFolder;
        if (!objFolder.empty()) return objFolder;

        auto base = L"/tmp";
        objFolder = Path::Combine(base, ObjectFolderDefaultW);
        Invariant(!objFolder.empty());
        return objFolder;
    }

#endif

} // end namespace Common
