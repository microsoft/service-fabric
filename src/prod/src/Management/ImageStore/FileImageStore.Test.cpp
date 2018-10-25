// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define ONE_MEGABYTE 1024 * 1024

using namespace std;
using namespace Common;
using namespace Management;
using namespace ServiceModel;
using namespace ImageModel;
using namespace ImageStore;
using namespace FileStoreService;

StringLiteral TraceTag("ImageStoreTest");

class FileImageStoreTests
{
protected:
    FileImageStoreTests() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~FileImageStoreTests() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    void GenerateFile(wstring const & fileName);
    void GenerateLargeFile(wstring const & fileName);
    void InitializeTest();

    void DeleteDirectory(wstring const & dirName);
    void DeleteFile(wstring const & fileName);
    void CleanupFoldersAndFiles();

    static GlobalString Content;
    static GlobalWString ImageCacheRoot;    
    static GlobalWString StoreDir;
    static GlobalWString Dir1;
    static GlobalWString File1;
    static GlobalWString File2;
    static GlobalWString DirFile1;
    static GlobalWString DirFile2;
    static GlobalWString Dir2;
    static GlobalWString Dir2File1;
    static GlobalWString Dir2File2;
    static GlobalWString Dir12;
    static GlobalWString Dir12File1;

    static Global<SecureString> StoreRootUri;
};

GlobalString FileImageStoreTests::Content = make_global<string>("Content");
GlobalWString FileImageStoreTests::ImageCacheRoot = make_global<wstring>(L"ImageStoreTestImageCache");
GlobalWString FileImageStoreTests::StoreDir = make_global<wstring>(L"ImageStoreTestStore");
GlobalWString FileImageStoreTests::Dir1 = make_global<wstring>(L"ImageStoreTestDir");
GlobalWString FileImageStoreTests::File1 = make_global<wstring>(L"ImageStoreTestFile1");
GlobalWString FileImageStoreTests::File2 = make_global<wstring>(L"ImageStoreTestFile2");
GlobalWString FileImageStoreTests::DirFile1 = make_global<wstring>(L"ImageStoreTestDir\\file1");
GlobalWString FileImageStoreTests::DirFile2 = make_global<wstring>(L"ImageStoreTestDir\\file2");
GlobalWString FileImageStoreTests::Dir2 = make_global<wstring>(L"ImageStoreTestDir\\dir");
GlobalWString FileImageStoreTests::Dir2File1 = make_global<wstring>(L"ImageStoreTestDir\\dir\\file1");
GlobalWString FileImageStoreTests::Dir2File2 = make_global<wstring>(L"ImageStoreTestDir\\dir\\file2");
GlobalWString FileImageStoreTests::Dir12 = make_global<wstring>(L"ImageStoreTestDir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\dir");
GlobalWString FileImageStoreTests::Dir12File1 = make_global<wstring>(L"ImageStoreTestDir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\dir\\file1");

Global<SecureString> FileImageStoreTests::StoreRootUri = make_global<SecureString>(L"file:ImageStoreTestStore");

void FileImageStoreTests::GenerateFile(wstring const & fileName)
{
    Trace.WriteInfo(TraceTag,
        L"GenerateFile",
        "Creating Directory {0}", 
        Path::GetDirectoryName(fileName));
    Directory::Create2(Path::GetDirectoryName(fileName));
    Trace.WriteInfo(TraceTag,
        L"GenerateFile",
        "Creating file {0}", 
        fileName);
    File f;
    auto error = f.TryOpen(fileName, FileMode::Create, FileAccess::Write, FileShare::None);
    Trace.WriteInfo(TraceTag,
        L"GenerateFile",
        "Opened file {0}:{1}", 
        fileName,
        error);
    VERIFY_IS_TRUE(error.IsSuccess());

    f.Write(Content->c_str(), (int) Content->size());
    f.Close();
}

void FileImageStoreTests::GenerateLargeFile(wstring const & fileName)
{
    int Array[1024];
    Trace.WriteInfo(TraceTag,
        L"GenerateFile",
        "Creating Directory:{0}", 
        Path::GetDirectoryName(fileName));
    Directory::Create2(Path::GetDirectoryName(fileName));
    Trace.WriteInfo(TraceTag,
        L"GenerateFile",
        "Creating file:{0}", 
        fileName);
    for (int i = 0; i < 1024; i++)
    {
        Array[i] = i;
    }
    File f;
    auto error = f.TryOpen(fileName, FileMode::Create, FileAccess::Write, FileShare::None);
    Trace.WriteInfo(TraceTag,
        L"GenerateLargeFile",
        "Opened file {0}:{1}", 
        fileName,
        error);
    VERIFY_IS_TRUE(error.IsSuccess());

    for (int i = 0; i < 700 * 1024; i++)
    {
        f.Write(Array, sizeof(int) * 1024);
    }
    f.Close();
}

void FileImageStoreTests::DeleteDirectory(wstring const & dirName)
{
    if (!Directory::Exists(dirName))
    {
        return;
    }

    auto error = Directory::Delete(dirName, true, true);
    Trace.WriteInfo(TraceTag,
        L"DeleteDirectory",
        "Delete {0} returned {1}",
        dirName,
        error);
    VERIFY_IS_TRUE(error.IsSuccess());
}

void FileImageStoreTests::DeleteFile(wstring const & fileName)
{
    if (!File::Exists(fileName))
    {
        return;
    }

    ::SetFileAttributes(fileName.c_str(), FILE_ATTRIBUTE_NORMAL);

    auto error = File::Delete2(fileName);
    Trace.WriteInfo(TraceTag,
        L"DeleteFile",
        "Delete {0} returned {1}",
        fileName,
        error);
    VERIFY_IS_TRUE(error.IsSuccess());
}

void FileImageStoreTests::CleanupFoldersAndFiles()
{
    DeleteFile(*File1);
    DeleteFile(*File2);
    DeleteDirectory(*ImageCacheRoot);
    DeleteDirectory(*StoreDir);
    DeleteDirectory(*Dir1);
}

void FileImageStoreTests::InitializeTest()
{
    Sleep(500);

    CleanupFoldersAndFiles();
   
    Trace.WriteInfo(TraceTag,
        L"Initialize",
        "Creating {0}", 
        *StoreDir);

    VERIFY_IS_TRUE(Directory::Create2(*StoreDir).IsSuccess());
    Trace.WriteInfo(TraceTag,
        L"Initialize",
        "Test Initialized.");
}

BOOST_FIXTURE_TEST_SUITE(FileImageStoreTestSuite, FileImageStoreTests)

BOOST_AUTO_TEST_CASE(TestRemoveRemoteContentNoCache)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestRemoveRemoteContentNoCache",
        "Start Test");
    InitializeTest();

    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");
    GenerateFile(Path::Combine(*StoreDir, *File1));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_TRUE(imageStoreUPtr->RemoveRemoteContent(*File1).IsSuccess());
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *File1)));

    GenerateFile(Path::Combine(*StoreDir, *File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));
    vector<wstring> objects;
    objects.push_back(*File1);
    objects.push_back(*Dir2);

    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(imageStoreUPtr->RemoveRemoteContent(objects).IsSuccess());
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
}

BOOST_AUTO_TEST_CASE(TestRemoveRemoteContentWithCache)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestRemoveRemoteContentWithCache",
        "Start Test");
    InitializeTest();

    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, *ImageCacheRoot);
    GenerateFile(Path::Combine(*StoreDir, *File1));
    GenerateFile(Path::Combine(*ImageCacheRoot, *File1));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_TRUE(imageStoreUPtr->RemoveRemoteContent(*File1).IsSuccess());
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *File1)));

    GenerateFile(Path::Combine(*StoreDir, *File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));
    GenerateFile(Path::Combine(*ImageCacheRoot, *File1));
    GenerateFile(Path::Combine(*ImageCacheRoot, *Dir2File1));
    GenerateFile(Path::Combine(*ImageCacheRoot, *Dir2File2));
    vector<wstring> objects;
    objects.push_back(*File1);
    objects.push_back(*Dir2);

    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_TRUE(imageStoreUPtr->RemoveRemoteContent(objects).IsSuccess());
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File2)));
}

BOOST_AUTO_TEST_CASE(TestUploadContentWithCache)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestUploadContentWithCache",
        "Start Test");
    InitializeTest();
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, *ImageCacheRoot);

    GenerateFile(*Dir2File1);
    GenerateFile(*Dir2File2);

    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_TRUE(imageStoreUPtr->UploadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File2)));

    InitializeTest();
    GenerateFile(*Dir2File1);
    GenerateFile(*Dir2File2);
    GenerateFile(*DirFile1);
    GenerateFile(*DirFile2);
    GenerateFile(*File1);
    GenerateFile(*File2);
    vector<wstring> objects;
    objects.push_back(*Dir1);
    objects.push_back(*File1);
    objects.push_back(*File2);

    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *DirFile1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *DirFile2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *File2)));
    VERIFY_IS_TRUE(imageStoreUPtr->UploadContent(objects, objects, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *DirFile1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *DirFile2)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *DirFile1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *DirFile2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*ImageCacheRoot, *File2)));
}

BOOST_AUTO_TEST_CASE(TestUploadContentNoCache)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestUploadContentNoCache",
        "Start Test");
    InitializeTest();
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");

    GenerateFile(*Dir2File1);
    GenerateFile(*Dir2File2);

    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_TRUE(imageStoreUPtr->UploadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));

    InitializeTest();
    GenerateFile(*Dir2File1);
    GenerateFile(*Dir2File2);
    GenerateFile(*DirFile1);
    GenerateFile(*DirFile2);
    GenerateFile(*File1);
    GenerateFile(*File2);
    vector<wstring> objects;
    objects.push_back(*Dir1);
    objects.push_back(*File1);
    objects.push_back(*File2);

    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *DirFile1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *DirFile2)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_FALSE(File::Exists(Path::Combine(*StoreDir, *File2)));
    VERIFY_IS_TRUE(imageStoreUPtr->UploadContent(objects, objects, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *Dir2File2)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *DirFile1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *DirFile2)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*StoreDir, *File2)));
}

BOOST_AUTO_TEST_CASE(TestDownloadContentWithCacheEcho)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestDownloadContentWithCacheEcho",
        "Start Test");
    // Download a directory (2 level deep) with echo
    InitializeTest();    
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, *ImageCacheRoot);

    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));

    VERIFY_IS_FALSE(File::Exists(*Dir2File1));
    VERIFY_IS_FALSE(File::Exists(*Dir2File2));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File2)));

    // Delete store and delete edit copied files and then copy from cache. (To ensure it does copy from cache.)
    VERIFY_IS_TRUE(Directory::Delete(*StoreDir, true).IsSuccess());
    VERIFY_IS_TRUE(Directory::Delete(*Dir2, true).IsSuccess());
    File::Delete(*Dir2File1, NOTHROW());
    GenerateFile(*Dir2File1);
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Echo).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));

    // Recreate the store (not the copied local version) and redownload to ensure it works.
    VERIFY_IS_TRUE(Directory::Create2(*StoreDir).IsSuccess());
    VERIFY_IS_TRUE(Directory::Delete(*ImageCacheRoot, true).IsSuccess());
    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Echo).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));

    // Delete cache and make sure it is being copied from store.
    VERIFY_IS_TRUE(Directory::Delete(*Dir2, true).IsSuccess());
    VERIFY_IS_TRUE(Directory::Delete(Path::Combine(*ImageCacheRoot, *Dir2), true).IsSuccess());
    File::Delete(*Dir2File1, NOTHROW());
    GenerateFile(*Dir2File1);
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Echo).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));

    // Delete store and delete copied directory and then copy from cache. (To ensure it does copy from cache.)
    VERIFY_IS_TRUE(Directory::Delete(*StoreDir, true).IsSuccess());
    VERIFY_IS_TRUE(Directory::Delete(*Dir2, true).IsSuccess());
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Echo).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));
}

BOOST_AUTO_TEST_CASE(TestDownloadContentWithCacheMultipleObjects)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestDownloadContentWithCacheMultipleObjects",
        "Start Test");
    InitializeTest();
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, *ImageCacheRoot);

    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));
    GenerateFile(Path::Combine(*StoreDir, *DirFile1));
    GenerateFile(Path::Combine(*StoreDir, *DirFile2));
    GenerateFile(Path::Combine(*StoreDir, *File1));
    GenerateFile(Path::Combine(*StoreDir, *File2));
    vector<wstring> objects;
    objects.push_back(*Dir1);
    objects.push_back(*File1);
    objects.push_back(*File2);

    VERIFY_IS_FALSE(File::Exists(*Dir2File1));
    VERIFY_IS_FALSE(File::Exists(*Dir2File2));
    VERIFY_IS_FALSE(File::Exists(*DirFile1));
    VERIFY_IS_FALSE(File::Exists(*DirFile2));
    VERIFY_IS_FALSE(File::Exists(*File1));
    VERIFY_IS_FALSE(File::Exists(*File2));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(objects, objects, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));
    VERIFY_IS_TRUE(File::Exists(*DirFile1));
    VERIFY_IS_TRUE(File::Exists(*DirFile2));
    VERIFY_IS_TRUE(File::Exists(*File1));
    VERIFY_IS_TRUE(File::Exists(*File2));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File2)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *DirFile1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *DirFile2)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *File2)));
}

BOOST_AUTO_TEST_CASE(TestDownloadContentWithCache12LeveDir)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestDownloadContentWithCache12LeveDir",
        "Start Test");
    // Download directory 12 level deep.
    InitializeTest();
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, *ImageCacheRoot);
    GenerateFile(Path::Combine(*StoreDir, *Dir12File1));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir12File1, *Dir12File1, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir12File1));
    VERIFY_IS_TRUE(Directory::Delete(*Dir1, true).IsSuccess());
}

BOOST_AUTO_TEST_CASE(TestDownloadContentWithCache)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestDownloadContentWithCache",
        "Start Test");
    // Download a directory (2 level deep);
    InitializeTest();
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, *ImageCacheRoot);

    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));

    VERIFY_IS_FALSE(File::Exists(*Dir2File1));
    VERIFY_IS_FALSE(File::Exists(*Dir2File2));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File1)));
    VERIFY_IS_TRUE(File::Exists(Path::Combine(*ImageCacheRoot, *Dir2File2)));

    // Delete store and delete copied directory and then copy from cache. (To ensure it does copy from cache.)
    VERIFY_IS_TRUE(Directory::Delete(*StoreDir, true).IsSuccess());
    VERIFY_IS_TRUE(Directory::Delete(*Dir2, true).IsSuccess());
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));

    // Recreate the store (not the copied local version) and redownload to ensure it works.
    // We have to delete the Cache too else copy will be from the cache.
    VERIFY_IS_TRUE(Directory::Create2(*StoreDir).IsSuccess());
    VERIFY_IS_TRUE(Directory::Delete(*ImageCacheRoot, true).IsSuccess());
    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));
    DateTime sourceTime, destTime;
    VERIFY_IS_TRUE(File::GetLastWriteTime(*Dir2File1, sourceTime).IsSuccess());
    VERIFY_IS_TRUE(File::GetLastWriteTime(Path::Combine(*StoreDir, *Dir2File1), destTime).IsSuccess());
    VERIFY_IS_TRUE(sourceTime == destTime);
    VERIFY_IS_TRUE(File::GetLastWriteTime(*Dir2File2, sourceTime).IsSuccess());
    VERIFY_IS_TRUE(File::GetLastWriteTime(Path::Combine(*StoreDir, *Dir2File2), destTime).IsSuccess());
    VERIFY_IS_TRUE(sourceTime == destTime);

    // Delete store and delete copied directory and then copy from cache. (To ensure it does copy from cache.)
    VERIFY_IS_TRUE(Directory::Delete(*StoreDir, true).IsSuccess());
    VERIFY_IS_TRUE(Directory::Delete(*Dir2, true).IsSuccess());
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));

    // Overwrite from the cache.
    VERIFY_IS_TRUE(Directory::Delete(*Dir2, true).IsSuccess());
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));
}

BOOST_AUTO_TEST_CASE(TestDownloadContentNoCache)
{
    Trace.WriteInfo(
        TraceTag,
        L"TestDownloadContentNoCache",
        "Start Test");
    InitializeTest();
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");

    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));

    VERIFY_IS_FALSE(File::Exists(*Dir2File1));
    VERIFY_IS_FALSE(File::Exists(*Dir2File2));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*Dir2, *Dir2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));

    InitializeTest();
    GenerateFile(Path::Combine(*StoreDir, *Dir2File1));
    GenerateFile(Path::Combine(*StoreDir, *Dir2File2));
    GenerateFile(Path::Combine(*StoreDir, *DirFile1));
    GenerateFile(Path::Combine(*StoreDir, *DirFile2));
    GenerateFile(Path::Combine(*StoreDir, *File1));
    GenerateFile(Path::Combine(*StoreDir, *File2));
    vector<wstring> objects;
    objects.push_back(*Dir1);
    objects.push_back(*File1);
    objects.push_back(*File2);

    VERIFY_IS_FALSE(File::Exists(*Dir2File1));
    VERIFY_IS_FALSE(File::Exists(*Dir2File2));
    VERIFY_IS_FALSE(File::Exists(*DirFile1));
    VERIFY_IS_FALSE(File::Exists(*DirFile2));
    VERIFY_IS_FALSE(File::Exists(*File1));
    VERIFY_IS_FALSE(File::Exists(*File2));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(objects, objects, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*Dir2File1));
    VERIFY_IS_TRUE(File::Exists(*Dir2File2));
    VERIFY_IS_TRUE(File::Exists(*DirFile1));
    VERIFY_IS_TRUE(File::Exists(*DirFile2));
    VERIFY_IS_TRUE(File::Exists(*File1));
    VERIFY_IS_TRUE(File::Exists(*File2));
}

BOOST_AUTO_TEST_CASE(TestEchoReDownload)
{
    InitializeTest();
    wstring storeFilePath = Path::Combine(*StoreDir, *File1);
    GenerateFile(storeFilePath);
    ::SetFileAttributes(storeFilePath.c_str(), FILE_ATTRIBUTE_READONLY);
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*File1, *File1, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Echo).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*File1));
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*File1, *File1, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Echo).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*File1));
    ::SetFileAttributes((*File1).c_str(), FILE_ATTRIBUTE_NORMAL);
    ::SetFileAttributes(storeFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
}

BOOST_AUTO_TEST_CASE(TestEchoReUpload)
{
    InitializeTest();
    wstring storeFilePath = Path::Combine(*StoreDir, *File1);
    GenerateFile(*File1);
    ::SetFileAttributes((*File1).c_str(), FILE_ATTRIBUTE_READONLY);
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");
    VERIFY_IS_TRUE(imageStoreUPtr->UploadContent(*File1, *File1, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Echo).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(storeFilePath));
    VERIFY_IS_TRUE(imageStoreUPtr->UploadContent(*File1, *File1, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Echo).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(storeFilePath));
    ::SetFileAttributes((*File1).c_str(), FILE_ATTRIBUTE_NORMAL);
    ::SetFileAttributes(storeFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
}

BOOST_AUTO_TEST_CASE(TestParallelUploads)
{
    InitializeTest();
    GenerateLargeFile(*File1);
    ::SetFileAttributes((*File1).c_str(), FILE_ATTRIBUTE_READONLY);
    GenerateLargeFile(*File2);
    ::SetFileAttributes((*File2).c_str(), FILE_ATTRIBUTE_READONLY);
    Common::AutoResetEvent eventToSet;
    bool otherThreadUploadSucceeded = false;
    bool thisThreadUploadSucceeded = false;
    Threadpool::Post([&eventToSet, &otherThreadUploadSucceeded]() { 
        ImageStoreUPtr imageStoreUPtr;
        wstring storeFilePath = Path::Combine(*StoreDir, *File1);
        ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");
        otherThreadUploadSucceeded = imageStoreUPtr->UploadContent(*File1, *File1, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess();
        if (otherThreadUploadSucceeded)
        {
            VERIFY_IS_TRUE(File::Exists(storeFilePath));
        }
        eventToSet.Set();
    });
    wstring storeFilePath = Path::Combine(*StoreDir, *File1);
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");
    thisThreadUploadSucceeded = imageStoreUPtr->UploadContent(*File1, *File2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess();
    if (thisThreadUploadSucceeded)
    {
        VERIFY_IS_TRUE(File::Exists(storeFilePath));
    }
    eventToSet.WaitOne();
    VERIFY_IS_TRUE(otherThreadUploadSucceeded != thisThreadUploadSucceeded);
    VERIFY_IS_TRUE(File::Exists(storeFilePath));
    VERIFY_IS_TRUE(imageStoreUPtr->UploadContent(*File1, *File2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(storeFilePath));
    ::SetFileAttributes(File2->c_str(), FILE_ATTRIBUTE_NORMAL);
    ::SetFileAttributes(File1->c_str(), FILE_ATTRIBUTE_NORMAL);
    ::SetFileAttributes(storeFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
}

BOOST_AUTO_TEST_CASE(TestParallelUploadsAndDownloads)
{
    InitializeTest();
    GenerateLargeFile(*File1);
    ::SetFileAttributes((*File1).c_str(), FILE_ATTRIBUTE_READONLY);
    
    Common::AutoResetEvent eventToSet;
    Common::AutoResetEvent startParallelDownload;
    
    Threadpool::Post([&eventToSet, &startParallelDownload]() { 
        ImageStoreUPtr imageStoreUPtr;
        ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");
        VERIFY_IS_TRUE(error.IsSuccess());

        startParallelDownload.Set();        
        
        VERIFY_IS_TRUE(imageStoreUPtr->UploadContent(*File1, *File1, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());

        wstring storeFilePath = Path::Combine(*StoreDir, *File1);
        VERIFY_IS_TRUE(File::Exists(storeFilePath));

        eventToSet.Set();
    }); 
    
    ImageStoreUPtr imageStoreUPtr;
    ErrorCode error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, StoreRootUri, 1, L"");
    VERIFY_IS_TRUE(error.IsSuccess());
        
    startParallelDownload.WaitOne();
    Sleep(250);    
    
    VERIFY_IS_FALSE(imageStoreUPtr->DownloadContent(*File1, *File2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    eventToSet.WaitOne();
    VERIFY_IS_TRUE(imageStoreUPtr->DownloadContent(*File1, *File2, ServiceModelConfig::GetConfig().MaxFileOperationTimeout, CopyFlag::Overwrite).IsSuccess());
    VERIFY_IS_TRUE(File::Exists(*File1));

    wstring storeFilePath = Path::Combine(*StoreDir, *File1);

    ::SetFileAttributes(File1->c_str(), FILE_ATTRIBUTE_NORMAL);
    ::SetFileAttributes(File2->c_str(), FILE_ATTRIBUTE_NORMAL);
    ::SetFileAttributes(storeFilePath.c_str(), FILE_ATTRIBUTE_NORMAL);
}

BOOST_AUTO_TEST_CASE(TestParallelDownloadArchive)
{
    InitializeTest();

    auto srcDirName = L"srcDir";
    auto srcDir = Path::Combine(*StoreDir, srcDirName);

    auto destRoot = *Dir1;
    auto destDir = Path::Combine(destRoot, L"destDir");

    int fileCount = 100;
    auto fileNamePrefix = L"testfile";

    for (auto ix=0; ix<fileCount; ++ix)
    {
        auto srcFile = Path::Combine(srcDir, wformatString("{0}{1}", fileNamePrefix, ix));

        GenerateFile(srcFile);
        ::SetFileAttributes(srcFile.c_str(), FILE_ATTRIBUTE_READONLY);
    }

    auto srcZip = ImageModelUtility::GetSubPackageArchiveFileName(srcDir);
    auto error = Directory::CreateArchive(srcDir, srcZip);
    VERIFY_IS_TRUE(error.IsSuccess());

    auto operationCount = 20;
    Common::ManualResetEvent downloadEvent;
    atomic_long pendingCount(operationCount);
    
    for (auto ix=0; ix<operationCount; ++ix)
    {
        Threadpool::Post([&downloadEvent, &pendingCount, &srcDirName, &destDir]() 
        { 
            ImageStoreUPtr imageStoreUPtr;
            auto error = ImageStoreFactory::CreateImageStore(imageStoreUPtr, *StoreRootUri, 1, *ImageCacheRoot);
            VERIFY_IS_TRUE(error.IsSuccess());

            bool retry = false;

            do
            {
                error = imageStoreUPtr->DownloadContent(
                    srcDirName,
                    destDir,
                    ServiceModelConfig::GetConfig().MaxFileOperationTimeout, 
                    L"", // remote checksum
                    L"", // expected checkum
                    true, // refresh cache
                    CopyFlag::Overwrite,
                    false, // local store layout only
                    true); // check for archive

                retry = (error.IsWin32Error(ERROR_SHARING_VIOLATION) || error.IsError(ErrorCodeValue::OperationFailed) || error.IsError(ErrorCodeValue::SharingAccessLockViolation));

                if (retry)
                {
                    Sleep(10);
                }

            } while (retry);
            
            Trace.WriteInfo(TraceTag, "DownloadContent({0}, {1}): {2}", srcDirName, destDir, error);

            VERIFY_IS_TRUE(error.IsSuccess());

            if (--pendingCount == 0)
            {
                downloadEvent.Set();
            }
        }); 
    }
    
    downloadEvent.WaitOne();

    for (auto ix=0; ix<fileCount; ++ix)
    {
        auto srcFile = Path::Combine(srcDir, wformatString("{0}{1}", fileNamePrefix, ix));
        VERIFY_IS_TRUE(File::Exists(srcFile));
        // Delete the files after validation
        DeleteFile(srcFile);
    }
    
    for (auto ix=0; ix<fileCount; ++ix)
    {
        auto destFile = Path::Combine(destDir, wformatString("{0}{1}", fileNamePrefix, ix));
        VERIFY_IS_TRUE(File::Exists(destFile));
        DeleteFile(destFile);
    }

    // Cleanup directories specific to this test
    DeleteDirectory(srcDir);
    DeleteDirectory(destDir);
}

BOOST_AUTO_TEST_SUITE_END()

bool FileImageStoreTests::TestSetup()
{
    Trace.WriteNoise(TraceTag, "++ Start test");
    // load tracing configuration
    Config cfg;
    Common::TraceProvider::LoadConfiguration(cfg);
    return true;
}

bool FileImageStoreTests::TestCleanup()
{
    CleanupFoldersAndFiles();
    Trace.WriteNoise(TraceTag, "++ Test Complete");
    return true;
}
