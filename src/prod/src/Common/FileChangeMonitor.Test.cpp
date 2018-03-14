// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


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

class FileChangeMonitorTest
{
protected:
    bool SetupTestCase(int count);
    bool CleanupTestCase();

    class TestContext;
    typedef shared_ptr<TestContext> TestContextSPtr;
    
    wstring ReadFromFile(wstring const & filepath);
    void WriteToFile(wstring const & filepath, wstring const & filetext);
    void TouchFile(wstring const &, int round);
    void VerifyNotificationCount(LONG expectedNotificationCount, TimeSpan waitTimeMax);
    
    vector<TestContextSPtr> contexts_;

    wstring testCaseFolder_;
    wstring testFilePath_;

    static wstring TestDirectoryName;
    static wstring TestFileName;
    static wstring TestFileContents;
};

class FileChangeMonitorTest::TestContext
    : public ComponentRoot
{
    DENY_COPY(TestContext)

public:
    TestContext()
        : ComponentRoot(),
        changeMonitor_(),
    changeNotificationCount_(0),
    readyToProcessFileChange_(false)
    {
    }

    virtual ~TestContext()
    {
    }

public:
    void OnChange(FileChangeMonitor2SPtr const & monitor);

public:
    std::shared_ptr<FileChangeMonitor2> changeMonitor_;
    Common::atomic_long changeNotificationCount_;
    AutoResetEvent readyToProcessFileChange_;
};

wstring FileChangeMonitorTest::TestDirectoryName = L"FileChangeMonitorTest.Data";
wstring FileChangeMonitorTest::TestFileName = L"MonitoredFile.txt";
wstring FileChangeMonitorTest::TestFileContents = L".";

BOOST_FIXTURE_TEST_SUITE2(FileChangeMonitor,FileChangeMonitorTest)

BOOST_AUTO_TEST_CASE(FileChangeMonitorTest1)
{
    ENTER;
    BOOST_REQUIRE(SetupTestCase(1));
    KFinally([this] { BOOST_REQUIRE(CleanupTestCase()); });

    TouchFile(testFilePath_, 1);
    this->VerifyNotificationCount(1l, TimeSpan::FromSeconds(30));

    LEAVE;
}

BOOST_AUTO_TEST_CASE(FileChangeMonitorTest2)
{
    ENTER;
    BOOST_REQUIRE(SetupTestCase(1));
    KFinally([this] { BOOST_REQUIRE(CleanupTestCase()); });

    auto const round = 50;
    for(int i = 1; i <= round; i++)
    {
        TouchFile(testFilePath_, i);
    }

    this->VerifyNotificationCount(round, TimeSpan::FromSeconds(30));
    LEAVE;
}

BOOST_AUTO_TEST_CASE(FileChangeMonitorTest3)
{
    ENTER;
    BOOST_REQUIRE(SetupTestCase(12));
    KFinally([this] { BOOST_REQUIRE(CleanupTestCase()); });

    static const int round = 3;
    for(int i = 1; i <= round; i++)
    {
        TouchFile(testFilePath_, i);
    }

    this->VerifyNotificationCount(round, TimeSpan::FromSeconds(30));
    LEAVE;
}

BOOST_AUTO_TEST_SUITE_END()

bool FileChangeMonitorTest::SetupTestCase(int count)
{
    CommonConfig::GetConfig();

    testCaseFolder_ = Path::Combine(TestDirectoryName, Guid::NewGuid().ToString());
    Trace.WriteInfo(TraceType, "creating test folder {0}", testCaseFolder_);
    auto error = Directory::Create2(testCaseFolder_);
    if (!error.IsSuccess()) 
    {
        FAIL_TEST("Failed to create test directory {0}: {1}", testCaseFolder_, error);
    }

    testFilePath_ = Path::Combine(testCaseFolder_, TestFileName);
    FileChangeMonitorTest::WriteToFile(testFilePath_, TestFileContents);

    for (int i = 0; i < count; ++i)
    {
        TestContextSPtr context = make_shared<TestContext>();
        context->changeNotificationCount_.store(0l);
        context->changeMonitor_ = FileChangeMonitor2::Create(testFilePath_);
        error = context->changeMonitor_->Open([context](FileChangeMonitor2SPtr const & monitor){ context->OnChange(monitor);} );

        if (!error.IsSuccess()) 
        {
            if (context->changeMonitor_->GetState() == FileChangeMonitor2::Failed)
            {
                context->changeMonitor_->Abort();
            }

            FAIL_TEST("Failed to open FileChangeMonitor for file {0}. ErrorCode={1}", testFilePath_, error);
        }

        context->readyToProcessFileChange_.Set();
        contexts_.emplace_back(move(context));
    }

    return true;
}

bool FileChangeMonitorTest::CleanupTestCase()
{
    for (auto const & context : contexts_)
    {
        if (context->changeMonitor_)
        {
            context->changeMonitor_->Abort();
            context->changeMonitor_.reset();
        }
    }

    contexts_.clear();
    return true;
}

void FileChangeMonitorTest::WriteToFile(wstring const & filePath, wstring const & fileText)
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

wstring FileChangeMonitorTest::ReadFromFile(wstring const & filePath)
{
    File file;
    auto error = file.TryOpen(filePath, FileMode::Open, FileAccess::Read);
    if (!error.IsSuccess())
    {
        FAIL_TEST("Failed to open '{0}' due to {1}", filePath, error);
    }

    int fileSize = static_cast<int>(file.size()+1); //+1 for string terminator
    vector<wchar_t> buffer(fileSize);
    int readCount = file.TryRead(buffer.data(), fileSize);
    if (!error.IsSuccess())
    {
        FAIL_TEST("Failed read file '{0}': size = {1} read = {2}", filePath, fileSize, readCount);
    }
    
    file.Close();

    return wstring(buffer.data());
}

void FileChangeMonitorTest::TouchFile(wstring const & filePath, int round)
{
    wstring fileContents = FileChangeMonitorTest::ReadFromFile(filePath);
    Sleep(30); // NTFS last write time has a resolution greater than 1 millisecond

    for(auto const & context : contexts_)
    {
        // Need to wait for the previous change to be processed, otherwise we may lose some changes. Our current
        // FileChangeMonitor implementation filters out "old" changes by keeping track of "last write time". A change
        // will be lost if a new change happens before the current one is processed.
        if (!context->readyToProcessFileChange_.WaitOne(TimeSpan::FromSeconds(60)))
        {
            BOOST_FAIL("readyToProcessFileChange_.WaitOne() returned false");
        }

        FileChangeMonitorTest::WriteToFile(filePath, fileContents);
        Trace.WriteInfo(
            TraceType, 
            "Touched file '{0}', round = {1}, change notification received = {2}", 
            filePath, 
            round, 
            context->changeNotificationCount_.load());
    }
}
void FileChangeMonitorTest::VerifyNotificationCount(LONG expectedNotificationCount, TimeSpan waitTimeMax)
{
    TimeSpan waitTime = TimeSpan::Zero;
    TimeSpan retryDelay = TimeSpan::FromMilliseconds(300);

    for(auto const & context : contexts_)
    {
        for(;;)
        {
            LONG currentNotificationCount = context->changeNotificationCount_.load();
			Trace.WriteInfo(
				TraceType,
				"currentNotificationCount = {0}, expected = {1}",
				currentNotificationCount,
				expectedNotificationCount);

            if (currentNotificationCount >= expectedNotificationCount) // todo, figure out how to eliminate ">" and use "=="
            {
                break;
            }

            waitTime = waitTime + retryDelay;
            if (waitTime > waitTimeMax)
            {
                FAIL_TEST(
                    "Notification counts do not match. Expected={0}, Actual={1}", 
                    expectedNotificationCount, 
                    currentNotificationCount);
            }
            else
            {
                Trace.WriteInfo(
                    TraceType,
                    "Sleep to wait for more change notifications: current = {0}, expected = {1}",
                    currentNotificationCount, expectedNotificationCount);

                Sleep(static_cast<DWORD>(retryDelay.TotalMilliseconds()));
            }
        }
    }
}

void FileChangeMonitorTest::TestContext::OnChange(FileChangeMonitor2SPtr const &)
{
    this->changeNotificationCount_++;
    this->readyToProcessFileChange_.Set();
}
