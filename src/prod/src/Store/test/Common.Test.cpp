// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Common/Directory.h"
#include "Store/test/Common.Test.h"

using namespace Common;
using namespace std;

namespace Store
{
    TemporaryDirectory::TemporaryDirectory(std::wstring const & baseName)
        : path_(TemporaryDirectory::GeneratePath(baseName))
    {
        if (Directory::Exists(path_))
        {
            auto error = Directory::Delete(path_, true);
            ASSERT_IFNOT(error.IsSuccess(), "Delete of path {0} failed due to {1}", path_, error);
        }

        Directory::Create(path_);
    }

    TemporaryDirectory::~TemporaryDirectory()
    {
        //
        // Virus scanners can mark files in use, and we have observed cases
        // where a database file is held for more than 30 seconds.
        //
        // Since we always delete the directory before using it if it exists
        // in the constructor, it is okay to leave the directory behind if it
        // contains files that are in use.
        //

        auto error = Directory::Delete(path_, true);
        if (!error.IsSuccess())
        {
            Trace.WriteError(Constants::StoreSource, "Error deleting {0} due to {1}, leaving directory behind", path_, error); 
        }
    }

    std::wstring TemporaryDirectory::GeneratePath(std::wstring const & baseName)
    {
        std::wstring path = Path::Combine(Directory::GetCurrentDirectoryW(), L"StoreTestTemp");

        return Path::Combine(path, baseName);
    }

#if !defined(PLATFORM_UNIX)
    TemporaryDatabase::TemporaryDatabase(TemporaryDirectory const & directory)
        : directory_(directory),
          filename_(L"temp_db.ese"),
          filepath_(Path::Combine(directory.Path, filename_)),
          settings_(make_shared<EseLocalStoreSettings>()),
          perfCounters_(EseStorePerformanceCounters::CreateInstance(wformatString("TemporaryDatabase_{0}", SequenceNumber::GetNext()))),
          instance_(EseInstance::CreateSPtr(settings_, perfCounters_, L"TemporaryFabricEse", filename_, directory_.Path)),
          session_(EseSession::CreateSPtr(instance_)),
          database_(EseDatabase::CreateSPtr(session_))
    {
        Trace.WriteInfo(Constants::StoreSource, "Opening ESE db: {0}", filepath_);

        auto error = instance_->Open();
        ASSERT_IFNOT(error.IsSuccess(), "Failed to open ESE db: {0}, error={1}", filepath_, error);

        Trace.WriteInfo(Constants::StoreSource, "Opened ESE db: {0}", filepath_);
    }

    TemporaryDatabase::~TemporaryDatabase()
    {
        Trace.WriteInfo(Constants::StoreSource, "Closing ESE db: {0}", filepath_);

        auto error = instance_->Close();
        ASSERT_IFNOT(error.IsSuccess(), "Failed to close ESE db: {0}, error={1}", filepath_, error);

        Trace.WriteInfo(Constants::StoreSource, "Closed ESE db: {0}", filepath_);
    }

    void TemporaryDatabase::InitializeCreate()
    {
        auto jetErr = session_->Initialize();
        ASSERT_IFNOT(jetErr == JET_errSuccess, "Failed to initialize ESE session: {0}, error={1}", filepath_, jetErr);
        {
            database_->InitializeCreate(filepath_.c_str());
        }
    }

    void TemporaryDatabase::InitializeAttachAndOpen()
    {
        auto jetErr = session_->Initialize();
        ASSERT_IFNOT(jetErr == JET_errSuccess, "Failed to initialize ESE session: {0}, error={1}", filepath_, jetErr);
        {
            database_->InitializeAttachAndOpen(filepath_.c_str());
        }
    }

    EseCursorUPtr TemporaryDatabase::CreateCursor()
    {
        return make_unique<EseCursor>(database_, session_);
    }
#endif

    TemporaryCurrentDirectory::TemporaryCurrentDirectory(
        std::wstring const & baseName)
        : TemporaryDirectory(baseName),
        originalCurrentDirectory_(Directory::GetCurrentDirectory())
    {
        // This trace ensures the trace file path is correctly set before changing the current directory
        Trace.WriteInfo("Store.TestTemporaryCurrentDirectory", "Setting current directory to {0}", this->Path);
        Directory::SetCurrentDirectory(this->Path);
    }

    TemporaryCurrentDirectory::~TemporaryCurrentDirectory()
    {
        Directory::SetCurrentDirectory(originalCurrentDirectory_);
    }
}
