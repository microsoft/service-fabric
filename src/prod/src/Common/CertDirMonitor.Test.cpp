// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <sys/stat.h>

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;

#define FAIL_TEST( fmt, ...) \
    { \
        wstring tmp; \
        StringWriter writer(tmp); \
        writer.Write(fmt, __VA_ARGS__); \
        Trace.WriteError(TraceType, "{0}", tmp); \
        VERIFY_IS_TRUE( false, tmp.c_str() ); \
    } \

class CertDirMonitorTest
{
protected:
    bool SetupTestCase();
    bool CleanupTestCase();
    void WriteToFile(wstring const & filepath, wstring const & filetext);

    wstring testCaseFolder_;
    wstring testSrcPath_;
    wstring testDestPath_;

    static wstring TestDirectoryName;
    static wstring TestSrcDirectoryName;
    static wstring TestDestDirectoryName;
    static wstring TestFileContents;
};

wstring CertDirMonitorTest::TestDirectoryName = L"CertDirMonitorTest.Data";
wstring CertDirMonitorTest::TestSrcDirectoryName = L"waagent";
wstring CertDirMonitorTest::TestDestDirectoryName = L"waagent_mirror";

BOOST_FIXTURE_TEST_SUITE2(CertDirMonitorSuite, CertDirMonitorTest)

BOOST_AUTO_TEST_CASE(CertMonitorTest1)
{
    ENTER;
    BOOST_REQUIRE(SetupTestCase());
    KFinally([this] { BOOST_REQUIRE(CleanupTestCase()); });

    CertDirMonitor &monitor = CertDirMonitor::GetSingleton();

    int i = 0;

    // prepare pre-existing files
    wstring preExistsFileNameBase = L"preExists";
    wstring preExistsFileContent = L"preExists Cert Content";
    for (i = 0; i < CertDirMonitor::NrCertFileExts; i++)
    {
        wstring preFilePath = Path::Combine(testSrcPath_, preExistsFileNameBase + L"." + CertDirMonitor::CertFileExtsW[i]);
        WriteToFile(preFilePath, preExistsFileContent);
    }

    // start monitoring
    ErrorCode error = monitor.StartWatch(StringUtility::Utf16ToUtf8(testSrcPath_), StringUtility::Utf16ToUtf8(testDestPath_));
    if (!error.IsSuccess())
    {
        FAIL_TEST("CertDirMonitor StartWatch Failed with error {0}. Directories: '{1}' to '{}'", error, testSrcPath_, testDestPath_);
    }

    // verify the copying of pre-existing files
    for (i = 0; i < CertDirMonitor::NrCertFileExts; i++)
    {
        wstring preFilePath = Path::Combine(testDestPath_, preExistsFileNameBase + L"." + CertDirMonitor::CertFileExtsW[i]);
        if (!File::Exists(preFilePath))
        {
            FAIL_TEST("verification failed: CertDirMonitor copying of pre-existing file {0}", preFilePath);
        }
    }

    // put in some new files
    wstring newFileNameBase = L"onWatch";
    wstring newFileContent = L"onWatch Cert Content";

    for (i = 0; i < CertDirMonitor::NrCertFileExts; i++)
    {
        wstring newFilePath = Path::Combine(testSrcPath_, newFileNameBase + L"." + CertDirMonitor::CertFileExtsW[i]);
        WriteToFile(newFilePath, newFileContent);
    }

    // add some noise on non-cert files, sub folders
    wstring noiseFileNameBase = L"noise";
    wstring noiseFileContent = L"noise Cert Content";
    wstring noiseDirPath = Path::Combine(testSrcPath_, Guid::NewGuid().ToString());
    wstring noiseFilePath = Path::Combine(noiseDirPath, noiseFileNameBase + L"." + CertDirMonitor::CertFileExtsW[0]);
    Directory::Create(noiseDirPath);
    WriteToFile(noiseFilePath, noiseFileContent);

    wstring noiseDirectFilePath = Path::Combine(testSrcPath_, Guid::NewGuid().ToString() + L".noise");
    WriteToFile(noiseDirectFilePath, noiseFileContent);

    // give it some time
    Sleep(1000);

    // verify
    for (i = 0; i < CertDirMonitor::NrCertFileExts; i++)
    {
        wstring newFilePath = Path::Combine(testDestPath_, newFileNameBase + L"." + CertDirMonitor::CertFileExtsW[i]);
        if (!File::Exists(newFilePath))
        {
            FAIL_TEST("verification failed: CertDirMonitor syncing of new file {0}", newFilePath);
        }
        struct stat newFileStat;
        string newFilePathA = StringUtility::Utf16ToUtf8(newFilePath);
        if (stat(newFilePathA.c_str(), &newFileStat) || (newFileStat.st_mode & (S_IWOTH))) {
            FAIL_TEST("verification failed: CertDirMonitor syncing of new file with mode check of {0}", newFilePath);
        }
    }

    // delete new files
    for (i = 0; i < CertDirMonitor::NrCertFileExts; i++)
    {
        wstring newFilePath = Path::Combine(testSrcPath_, newFileNameBase + L"." + CertDirMonitor::CertFileExtsW[i]);
        File::Delete(newFilePath);
    }

    Sleep(1000);

    // verify
    for (i = 0; i < CertDirMonitor::NrCertFileExts; i++)
    {
        wstring newFilePath = Path::Combine(testDestPath_, newFileNameBase + L"." + CertDirMonitor::CertFileExtsW[i]);
        if (File::Exists(newFilePath))
        {
            FAIL_TEST("verification failed: CertDirMonitor syncing of deleted file {0}", newFilePath);
        }
    }

    LEAVE;
}

BOOST_AUTO_TEST_SUITE_END()

bool CertDirMonitorTest::SetupTestCase()
{
    testCaseFolder_ = Path::Combine(TestDirectoryName, Guid::NewGuid().ToString());
    Trace.WriteInfo(TraceType, "creating test folder {0}", testCaseFolder_);
    auto error = Directory::Create2(testCaseFolder_);
    if (!error.IsSuccess())
    {
        FAIL_TEST("Failed to create test directory {0}: {1}", testCaseFolder_, error);
    }

    testSrcPath_ = Path::Combine(testCaseFolder_, TestSrcDirectoryName);
    error = Directory::Create2(testSrcPath_);
    if (!error.IsSuccess())
    {
        FAIL_TEST("Failed to create test src directory {0}: {1}", testSrcPath_, error);
    }

    testDestPath_ = Path::Combine(testCaseFolder_, TestDestDirectoryName);
    return true;
}

bool CertDirMonitorTest::CleanupTestCase()
{
    return true;
}

void CertDirMonitorTest::WriteToFile(wstring const & filePath, wstring const & fileText)
{
    wstring directoryName = Path::GetDirectoryName(filePath);
    wstring tempFileName = Guid::NewGuid().ToString();
    wstring tempFilePath = Path::Combine(directoryName, tempFileName);

    File file;
    auto error = file.TryOpen(tempFilePath, FileMode::Create, FileAccess::Write, FileShare::ReadWrite);
    if (!error.IsSuccess())
    {
        File::Delete(tempFilePath, Common::NOTHROW());
        FAIL_TEST("Failed to open '{0}' due to {1}", filePath, error);
    }


    int size = static_cast<int>(fileText.length() * sizeof(wchar_t));
    int written = file.TryWrite((void*)fileText.c_str(), size);
    if (!error.IsSuccess())
    {
        File::Delete(tempFilePath, Common::NOTHROW());
        FAIL_TEST("Failed write file '{0}': size = {1} written = {2}", filePath, size, written);
    }

    file.Flush();
    file.Close();

    File::Copy(tempFilePath, filePath, true);
    File::Delete(tempFilePath, Common::NOTHROW());
}
