// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once // intellisense workaround

namespace Common
{
    typedef std::map<std::wstring, std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> EnvironmentMap;

    class Environment :
        Common::TextTraceComponent<Common::TraceTaskCodes::Common>
    {
    public:

        static std::wstring const NewLine;

        //
        // Get the environment variable
        //
        static void GetEnvironmentVariable( std::wstring const & name,
                                            std::wstring& outValue,
                                            NOCLEAR );
        static bool GetEnvironmentVariable( std::wstring const & name,
                                            std::wstring& outValue,
                                            NOTHROW );
        static void GetEnvironmentVariable( std::wstring const & name,
                                            std::wstring& outValue );

        static void GetEnvironmentVariable( std::wstring const & name,
                                            std::wstring& outValue,
                                            std::wstring const & defaultValue );

        //
        // A GetEnvironmentVariable that conforms to the newer cxl way of doing things
        //
        static std::wstring GetEnvironmentVariable ( std::wstring const & name );


        static bool SetEnvironmentVariable(std::wstring const & name, std::wstring const& value);

        static std::wstring GetMachineName() { std::wstring result; GetMachineName(result); return result; }
        static void GetMachineName( std::wstring& buffer);
        static void GetMachineFQDN( std::wstring& buffer);

        static void GetUserName(std::wstring & buffer);
        static void GetExecutablePath(std::wstring & buffer);
        static void GetExecutableFileName(std::wstring & buffer);
        static std::wstring GetExecutableFileName();
        static std::wstring GetExecutablePath();
        static void GetCurrentModuleFileName(std::wstring & buffer);
        static std::wstring GetCurrentModuleFileName() {std::wstring result; GetCurrentModuleFileName(result); return result;}
        static void GetModuleFileName(HMODULE module, std::wstring & buffer);
        static DWORD GetNumberOfProcessors();
        static Common::ErrorCode Environment::GetAvailableMemoryInBytes(__out DWORDLONG& memoryInMB);

        static Common::ErrorCode GetCurrentModuleFileName2(__out std::wstring & moduleFileName);
        static Common::ErrorCode GetModuleFileName2(HMODULE module, __out std::wstring & moduleFileName);
        static Common::DateTime GetLastRebootTime();

        static bool ExpandEnvironmentStrings(std::wstring const & str, __out std::wstring & expandedStr);
        static Common::ErrorCode Expand(std::wstring const & str, __out std::wstring & expandedStr);
        static std::wstring Expand(std::wstring const & str);
        static bool GetEnvironmentMap(__out EnvironmentMap & envMap);

#if !defined(PLATFORM_UNIX)
        static DWORD_PTR GetActiveProcessorsMask();
        static std::wstring GetCurrentModuleFileVersion() { std::wstring result; GetCurrentModuleFileVersion2(result); return result; }
        static Common::ErrorCode GetFileVersion2(__in std::wstring const & fileName, __out std::wstring & fileVersion);
        static Common::ErrorCode GetCurrentModuleFileVersion2(__out std::wstring & fileVersion);
        static Common::ErrorCode GetEnvironmentMap(HANDLE userTokenHandle, bool inherit, __out EnvironmentMap & envMap);
#endif
        static void ToEnvironmentBlock(EnvironmentMap const & envMap, __out std::vector<wchar_t> & envBlock);
        static void FromEnvironmentBlock(LPVOID envBlock, __out EnvironmentMap & envMap);
        static std::wstring ToString(EnvironmentMap const & envMap);

        static std::string GetHomeFolder();
        static std::wstring GetHomeFolderW();

        static std::wstring GetObjectFolder();
    };

    class EnvironmentVariable
    {
    public:
        EnvironmentVariable(
            const std::wstring &   name,
            const std::wstring &   defaultValue = L"" )
        {
            ASSERT_IF( name.empty(), "environment variable empty");
            
            this->name = name;
            
            Environment::GetEnvironmentVariable( name, currentValue, NOTHROW() );

            if ( currentValue.empty() && !defaultValue.empty() )
            {
                *this = defaultValue;
            }
        } // ctor
        
        std::wstring const& ToString() const
        {
            return currentValue;
        }

        bool Equals( const std::wstring & rhs ) const
        {
            return rhs == currentValue;
        }

        static std::wstring AppendDirectoryToPathEnvVarValue(std::wstring const &currentPathValue, std::wstring const &newDirectory);

        EnvironmentVariable & operator = (const std::wstring & newValue);

        __declspec( property( get=size )) int Count;
        int size()    { return (int)currentValue.size(); }

        __declspec( property( get=empty )) bool IsEmpty;
        bool empty() { return currentValue.empty(); }

    protected:
        std::wstring           name;
        std::wstring           currentValue;
    }; // EnvironmentVariable

};
