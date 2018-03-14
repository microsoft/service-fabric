// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;


const StringLiteral TraceType("CachedFileTest");

#define THREADCOUNT 10

#define FAIL_TEST( fmt, ...) \
    { \
        wstring tmp; \
        StringWriter writer(tmp); \
        writer.Write(fmt, __VA_ARGS__); \
        Trace.WriteError(TraceType, "{0}", tmp); \
        VERIFY_IS_TRUE( false, tmp.c_str() ); \
    } \

#define LOG_INFO( fmt, ...) \
    { \
        wstring tmp; \
        StringWriter writer(tmp); \
        writer.Write(fmt, __VA_ARGS__); \
        Trace.WriteInfo(TraceType, "{0}", tmp); \
    } \

class CachedFileTest
{
protected:
    CachedFileTest() { BOOST_REQUIRE(SetupTestCase()); }
    ~CachedFileTest() { BOOST_REQUIRE(CleanupTestCase()); }

    TEST_METHOD_SETUP(SetupTestCase);
    TEST_METHOD_CLEANUP(CleanupTestCase);

    // TEST_METHOD(CachedFileTest2);  /* Checks the consistency of reader's data with one writer and multiple readers */

    friend class CachedFile;
    Common::ErrorCode ReadFromCache(wstring & );
    Common::ErrorCode WriteToFile(wstring const & filepath, wstring const & filetext);
    void VerifyContent(wstring const &, TimeSpan const &);
    
    CachedFileSPtr cachedFile_;
    wstring expectedFileContent_;

    static wstring TestDirectoryName;
    static wstring TestFileName;
    static wstring TestFileContents;
};

wstring CachedFileTest::TestDirectoryName = L"CachedFileTest.Data";
wstring CachedFileTest::TestFileName = L"CachedFile.txt";
wstring CachedFileTest::TestFileContents = L"This is a Cached file.";


BOOST_FIXTURE_TEST_SUITE(CachedFileTestSuite,CachedFileTest)

BOOST_AUTO_TEST_CASE(CachedFileTest1)
{
    wstring testFilePath = Path::Combine(TestDirectoryName, TestFileName);
    wstring expectedContent = TestFileContents;
   
    VerifyContent(expectedContent, TimeSpan::FromSeconds(30));

    // Write Multiple times.
    int writeCount = 10;
    for(int i = 0; i < writeCount; ++i)
    {
        expectedContent.append(TestFileContents);
        WriteToFile(testFilePath,expectedContent);
    }

    VerifyContent(expectedContent, TimeSpan::FromSeconds(30));
}

BOOST_AUTO_TEST_SUITE_END()

bool CachedFileTest::SetupTestCase()
{
    CommonConfig::GetConfig();

    if (Directory::Exists(TestDirectoryName))
    {
        auto error = Directory::Delete(TestDirectoryName, true);
        if (!error.IsSuccess()) 
        {
            FAIL_TEST("Failed to delete test directory {0} with error {1}", TestDirectoryName, error.ReadValue()); 
            return false;
        }
    }

    auto error = Directory::Create2(TestDirectoryName);
    if (!error.IsSuccess()) 
    {
        FAIL_TEST("Failed to create test directory {0} with error {1}", TestDirectoryName, error.ReadValue());
        return false;
    }
    
    wstring filePath = Path::Combine(TestDirectoryName, TestFileName);
    CachedFileTest::WriteToFile(filePath, TestFileContents);

    error = CachedFile::Create(filePath,
            [] (__in wstring const & filePath, __out wstring & content)
        {
            ErrorCode error(ErrorCodeValue::Success);
            File file;
            // Open the file with sharing mode as None so that no one write into it while the read is in progress.
            error = file.TryOpen(filePath, FileMode::Open, FileAccess::Read, FileShare::None);
            if(!error.IsSuccess())
            {
                return error;
            }
    
            int fileSize = static_cast<int>(file.size());
            vector<wchar_t> buffer(fileSize);
            DWORD bytesRead;
            error = file.TryRead2(reinterpret_cast<void*>(buffer.data()), fileSize, bytesRead);

            file.Close();

            if (!error.IsSuccess())
            {
                return error;
            }

            content = wstring(buffer.data());
            return error;
        },
        cachedFile_);

    if(!error.IsSuccess())
    {
        FAIL_TEST("Failed to initialize cached file, Error {0}", error.ReadValue());
        return false;
    }

    return true;
}
bool CachedFileTest::CleanupTestCase()
{
    this->cachedFile_ = nullptr;
    return true;
}
ErrorCode CachedFileTest::WriteToFile(wstring const & filePath, wstring const & fileText)
{
    wstring directoryName = Path::GetDirectoryName(filePath);
    wstring tempFileName = Guid::NewGuid().ToString();
    wstring tempFilePath = Path::Combine(directoryName, tempFileName);
    int currentRetryCount = 0;
    int maxRetryCount = 10;
    ErrorCode error  = ErrorCodeValue::Success;

    do
    {
        if(currentRetryCount)
        {
            LOG_INFO("CachedFileTest::WriteToFile | RetryCount:{0} , Sleeping..", currentRetryCount);
            // Sleep for 10 seconds 
            Sleep(10 * 1000);
        }
        File file;
        error = file.TryOpen(tempFilePath, FileMode::Create, FileAccess::Write, FileShare::ReadWrite);
        if (error.IsSuccess())
        {
            int size = static_cast<int>(fileText.length()) * 2;
            int written = file.TryWrite((void*)fileText.c_str(), size);
            if (error.IsSuccess())
            {
                written;
                file.Flush();
                file.Close();

                File::Copy(tempFilePath, filePath, true);
                File::Delete(tempFilePath, Common::NOTHROW());
            }
            else
            {
                File::Delete(tempFilePath, Common::NOTHROW());
                
            }
        }
        else
        {
            File::Delete(tempFilePath, Common::NOTHROW());
        }    
    } while(error.ReadValue() == ErrorCodeValue::AccessDenied
        && currentRetryCount++ < maxRetryCount);

    if(error.ReadValue() != ErrorCodeValue::Success)
    {
        FAIL_TEST("Failed write file '{0}'", filePath);
    }
    return error;
}
ErrorCode CachedFileTest::ReadFromCache(wstring & value)
{
    return cachedFile_->ReadFileContent(value);
}
void CachedFileTest::VerifyContent(wstring const & expectedContent, TimeSpan const & waitTimeMax)
{
    TimeSpan waitTime = TimeSpan::Zero;
    TimeSpan retryDelay = TimeSpan::FromMilliseconds(300);

    for(;;)
    {
        wstring actualContent = L"";
        ReadFromCache(actualContent);
        if (actualContent == expectedContent)
        {
            break;
        }

        waitTime = waitTime + retryDelay;
        if (waitTime > waitTimeMax)
        {
            FAIL_TEST(
                "Actual Content '{0}' does not match expected Content '{1}'", 
                actualContent, 
                expectedContent);
        }
        else
        {
            Trace.WriteInfo(
                TraceType,
                "Sleep to wait for notifications to be processed");

            Sleep(static_cast<DWORD>(retryDelay.TotalMilliseconds()));
        }
    }
}

 // Single writer and multiple readers. 
 // Writer continuously writes two strings into a file.
 // Readers continuously read the value from the cache and check the consistency of the data.
 // The data should be either updated or old (i.e. one of the two values that the writer wrote)
//void CachedFileTest::CachedFileTest2()
//{
//    wstring testFilePath = Path::Combine(TestDirectoryName, TestFileName);
//   
//    // Run for 1 minutes
//    int executionTimeInMilliSeconds = 1* 60 * 1000;
//    wstring data[2];
//    data[0] = L"This is a Cached file.";
//    data[1] = L"This is a modified Cached file.";
//    vector<unique_ptr<ManualResetEvent>> readThreadEvents(THREADCOUNT);
//    ManualResetEvent writeThreadEvent(false);
//    atomic_long readErrorCount(0L);
//    long writeErrorCount = 0;
//    // A flag to terminate the threads
//    atomic_bool threadStopFlag(false);
//
//    int i = 0;
//    // Post a write thread
//    Threadpool::Post(
//            [this,&testFilePath, &writeErrorCount,&threadStopFlag,&writeThreadEvent,&i,&data]() -> void
//        {
//            ErrorCode error = ErrorCodeValue::Success;
//            wstring writeContent;
//            
//            while(!threadStopFlag.load())
//            {
//                writeContent = data[(i++ % 2)];
//                error = this->WriteToFile(testFilePath,writeContent);
//                if(!error.IsSuccess())
//                {
//                    FAIL_TEST("Failed ; Write failed");
//                    threadStopFlag.swap(true);
//                    writeErrorCount += 1;
//                    break;
//                }
//                
//                // Sleep for 1 second
//                //Sleep(1 * 1000);
//            }
//
//            writeThreadEvent.Set();
//        });
//   
//    // Post read threads
//    for(int i = 0; i < THREADCOUNT ; ++i)
//    {
//        readThreadEvents[i] = make_unique<ManualResetEvent>();
//        readThreadEvents[i]->Reset();
//        ManualResetEvent *eventPtr = readThreadEvents[i].get();
//        Threadpool::Post(
//            [this,&readErrorCount,eventPtr,&threadStopFlag,&data]() -> void
//        {
//            wstring actualContent;
//            ErrorCode error = ErrorCodeValue::Success;
//            while(!threadStopFlag.load())
//            {
//                error = this->ReadFromCache(actualContent);
//                if(!error.IsSuccess())
//                {
//                    // Readers are allowed to fail after using all retry counts.
//                    LOG_INFO("ReadFromCache failed for thread:{0} with error:{1}",GetCurrentThreadId(),error);
//                }
//                else if(actualContent != data[0] && actualContent != data[1])
//                {
//                    LOG_INFO("actual content:'{0}' did not match either:'{1}' or '{2}'",actualContent,data[0],data[1]);
//                    readErrorCount++;
//                }
//                        
//                //Sleep(1 * 1000);
//            }
//            
//            eventPtr->Set();
//        });
//    }
//
//    Sleep(executionTimeInMilliSeconds);
//
//    // Set the thread termination flag
//    threadStopFlag.swap(true);
//
//    // Wait for the write event
//    writeThreadEvent.WaitOne();
//
//    // Wait for the read events
//    for(int i = 0; i < THREADCOUNT; i++)
//    {
//        readThreadEvents[i]->WaitOne();
//    }
//
//    if(writeErrorCount != 0)
//    {
//        FAIL_TEST("Writer Thread failed");
//    }
//
//    if(readErrorCount.load() != 0)
//    {
//        FAIL_TEST("One or more reader Threads failed");
//    }
//}
