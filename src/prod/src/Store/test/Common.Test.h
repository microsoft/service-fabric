// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define TraceType "TransportTest"

namespace Store
{
    // Classes shared in common across test code
    class TemporaryDirectory
    {
    public:
        TemporaryDirectory(std::wstring const & baseName);
        ~TemporaryDirectory();

        __declspec(property(get=get_Path)) std::wstring const & Path;
        std::wstring const & get_Path() const { return path_; }

    private:
        std::wstring path_;

        static std::wstring GeneratePath(std::wstring const & baseName);
    };

    class TemporaryCurrentDirectory : public TemporaryDirectory
    {
    public:
        TemporaryCurrentDirectory(std::wstring const & baseName);
        ~TemporaryCurrentDirectory();
    private:
        std::wstring originalCurrentDirectory_;
    };

#if !defined(PLATFORM_UNIX)
    class TemporaryDatabase
    {
        DENY_COPY(TemporaryDatabase);

    public:
        TemporaryDatabase(TemporaryDirectory const & directory);
        ~TemporaryDatabase();

        __declspec(property(get=get_Instance)) EseInstance & Instance;
        __declspec(property(get=get_Session)) EseSession & Session;
        __declspec(property(get=get_Database)) EseDatabase & Database;

        EseInstance & get_Instance() { return *instance_; }
        EseSession & get_Session() { return *session_; }
        EseDatabase & get_Database() { return *database_; }

        void InitializeCreate();
        void InitializeAttachAndOpen();

        EseCursorUPtr CreateCursor();

    private:
        TemporaryDirectory const & directory_;
        std::wstring filename_;
        std::wstring filepath_;
        EseLocalStoreSettingsSPtr settings_;
        EseStorePerformanceCountersSPtr perfCounters_;
        EseInstanceSPtr instance_;
        EseSessionSPtr session_;
        EseDatabaseSPtr database_;
    };
#endif
}
