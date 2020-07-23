// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ServiceModel;
using namespace Management::ImageModel;
using namespace Management::ImageStore;

const StringLiteral TestSource("ImageStoreUtilityTest");

#define VERIFY( condition, fmt, ...) \
    { \
    wstring tmp; \
    StringWriter writer(tmp); \
    writer.Write(fmt, __VA_ARGS__); \
    VERIFY_IS_TRUE(condition, tmp.c_str()); \
    } \

class ImageStoreUtilityTests
{
protected:
    ImageStoreUtilityTests() { BOOST_REQUIRE(TestSetup()); }
    TEST_METHOD_SETUP(TestSetup);
    ~ImageStoreUtilityTests() { BOOST_REQUIRE(TestCleanup()); }
    TEST_METHOD_CLEANUP(TestCleanup);

    //
    // Helper methods
    //

    void GenerateAppPackage(wstring const & appPackageName);

    void GenerateSfpkgWithValidation(
        wstring const & appPackageRootDirectory,
        wstring const & destinationDirectory,
        bool applyCompression,
        wstring const & sfpkgName,
        ErrorCode const & expectedError,
        __out wstring & sfpkgFilePath);

    void ExpandSfpkgWithValidation(
        wstring const & sfpkgFilePath,
        wstring const & destinationDirectory,
        bool sfpkgShouldBeDeleted,
        ErrorCode const & expectedError);

    void CreateNewDirectory(wstring const & folderName);
    void DeleteDirectory(wstring const & folderName);
};

BOOST_FIXTURE_TEST_SUITE(ImageStoreUtilityTestSuite, ImageStoreUtilityTests)

BOOST_AUTO_TEST_CASE(TestSfpkgNegativeTestCases)
{
    Trace.WriteInfo(TestSource, L"TestSfpkgNegativeTestCases", "Start Test");

    wstring rootFolder = Path::GetFullPath(L"TestSfpkgNegativeTestCases");
    CreateNewDirectory(rootFolder);

    wstring sfPkgDestinationFolder(Path::Combine(rootFolder, L"SfpkgFolder"));
    wstring sfpkgFilePath;

    //
    // Generate sfpkg negative test cases
    // 
    
    // Generate sfpkg from non-existing folder.
    GenerateSfpkgWithValidation(
        Path::GetFullPath(L"NonExistingFolder"),
        sfPkgDestinationFolder,
        false /*applyCompression*/,
        L"",
        ErrorCode(ErrorCodeValue::DirectoryNotFound),
        sfpkgFilePath);

    // Generate the application package in the source folder.
    wstring appPackageName(L"AppPackage");
    wstring appPackageFolder(Path::Combine(rootFolder, appPackageName));
    GenerateAppPackage(appPackageFolder);

    // Generate in the same folder is not supported.
    GenerateSfpkgWithValidation(
        appPackageFolder,
        appPackageFolder,
        false /*applyCompression*/,
        L"",
        ErrorCode(ErrorCodeValue::InvalidArgument),
        sfpkgFilePath);

    // Inner folder inside appPackageFolder
    wstring appPackageInnerFolder(Path::Combine(appPackageFolder, L"Inner"));
    // Generate in a child folder is not supported.
    GenerateSfpkgWithValidation(
        appPackageFolder,
        appPackageInnerFolder,
        false /*applyCompression*/,
        L"",
        ErrorCode(ErrorCodeValue::InvalidArgument),
        sfpkgFilePath);

    //
    // Expand sfpkg negative test cases
    // 

    // Expand sfpkg that doesn't exist.
    ExpandSfpkgWithValidation(
        L"NonExisting.sfpkg",
        sfPkgDestinationFolder,
        false /*sfpkgShouldBeDeleted*/,
        ErrorCode(ErrorCodeValue::FileNotFound));

    // Expand file with another extension fails because the sfpkg is not valid.
    ExpandSfpkgWithValidation(
        L"NonExisting.other",
        sfPkgDestinationFolder,
        false /*sfpkgShouldBeDeleted*/,
        ErrorCode(ErrorCodeValue::InvalidArgument));
    
    // Cleanup
    DeleteDirectory(appPackageInnerFolder);
    DeleteDirectory(appPackageFolder);
    DeleteDirectory(sfPkgDestinationFolder);
    DeleteDirectory(rootFolder);
}

BOOST_AUTO_TEST_CASE(TestSfpkg)
{
    Trace.WriteInfo(TestSource, L"TestSfpkg", "Start Test");

    wstring rootFolder(L"TestSfpkgFolder");
    CreateNewDirectory(rootFolder);

    // Generate app package.
    wstring appPackageName(L"AppPackage");
    wstring appPackageFolder(Path::Combine(rootFolder, appPackageName));
    GenerateAppPackage(appPackageFolder);

    wstring sfPkgDestinationFolder(Path::Combine(rootFolder, L"SfpkgFolder"));
    // Inner folder inside appPackageFolder
    wstring appPackageInnerFolder(Path::Combine(appPackageFolder, L"Inner"));
    wstring processSfpkgFolder(Path::Combine(rootFolder, L"ProcessSfpgk"));
    wstring processSfpkgFolder2(Path::Combine(rootFolder, L"ProcessSfpkg2"));
    
    wstring sfpkgFilePath;

    // Generate sfpkg from the app folder to sfpkgDestination folder.
    // sfpkgDestinationFolder contains the .sfpkg, processSfpkg folders are empty.
    GenerateSfpkgWithValidation(
        appPackageFolder,
        sfPkgDestinationFolder,
        false /*applyCompression*/,
        L"",
        ErrorCode::Success(),
        sfpkgFilePath);

    // Expand to a folder that doesn't exist, the folder is created.
    ExpandSfpkgWithValidation(
        sfpkgFilePath,
        processSfpkgFolder,
        false /*sfpkgShouldBeDeleted*/,
        ErrorCode(ErrorCodeValue::Success));

    // Expand to the same folder fails because the folder is not empty.
    ExpandSfpkgWithValidation(
        sfpkgFilePath,
        processSfpkgFolder,
        false /*sfpkgShouldBeDeleted*/,
        ErrorCode(ErrorCodeValue::InvalidState));

    // Generate in a different folder, that doesn't exist. The sfpkg name is provided with different extension.
    // sfpkgDestinationFolder contains the .sfpkg, processSfpkgFolder contains expanded package, processSfpkgFolder2 has sfpkg without compression.
    wstring sfpkgFilePath2;
    GenerateSfpkgWithValidation(
        processSfpkgFolder,
        processSfpkgFolder2,
        false /*applyCompression*/,
        L"Test.changes",
        ErrorCode(ErrorCodeValue::Success),
        sfpkgFilePath2);
    int64 size;
    auto error = File::GetSize(sfpkgFilePath2, size);
    VERIFY(error.IsSuccess(), "File::GetSize(sfpkgFilePath2, size) failed with {0}.", error);

    // Generate to a folder that exists, sfpkg is overwritten.
    // sfpkgDestinationFolder contains the .sfpkg, processSfpkgFolder contains expanded package, processSfpkgFolder2 has sfpkg with compression.
    GenerateSfpkgWithValidation(
        processSfpkgFolder,
        processSfpkgFolder2,
        true /*applyCompression*/,
        L"Test.sfpkg",
        ErrorCode(ErrorCodeValue::Success),
        sfpkgFilePath2);
    int64 size2;
    error = File::GetSize(sfpkgFilePath2, size2);
    VERIFY(error.IsSuccess(), "File::GetSize(sfpkgFilePath2, size2) failed with {0}.", error);

    // The sizes are different because the sfpkgs have different compression
    VERIFY(size != size2, "size {0} != size2 {1}", size, size2);

    // Expand an sfpkg to a folder that contains only an sfpkg fails if the sfpkg name is not same.
    ExpandSfpkgWithValidation(
        sfpkgFilePath,
        processSfpkgFolder2,
        true /*sfpkgShouldBeDeleted*/,
        ErrorCode(ErrorCodeValue::InvalidState));

    // Expand in the same folder, sfpkg is deleted to ensure the package is correct.
    // sfpkgDestinationFolder contains the .sfpkg, processSfpkgFolder contains expanded package, processSfpkgFolder2 has expanded package.
    ExpandSfpkgWithValidation(
        sfpkgFilePath2,
        processSfpkgFolder2,
        true /*sfpkgShouldBeDeleted*/,
        ErrorCode(ErrorCodeValue::Success));
        
    // Cleanup
    DeleteDirectory(appPackageInnerFolder);
    DeleteDirectory(appPackageFolder);
    DeleteDirectory(sfPkgDestinationFolder);
    DeleteDirectory(processSfpkgFolder);
    DeleteDirectory(processSfpkgFolder2);
    DeleteDirectory(rootFolder);
}

BOOST_AUTO_TEST_SUITE_END()

void ImageStoreUtilityTests::GenerateSfpkgWithValidation(
    wstring const & appPackageRootDirectory,
    wstring const & destinationDirectory,
    bool applyCompression,
    wstring const & sfpkgName,
    ErrorCode const & expectedError,
    __out wstring & sfpkgFilePath)
{
    wstring expectedSfpkgName = sfpkgName.empty() ? Path::GetFileName(appPackageRootDirectory) : sfpkgName;
    Path::ChangeExtension(expectedSfpkgName, L".sfpkg");

    auto error = ImageStoreUtility::GenerateSfpkg(
        appPackageRootDirectory,
        destinationDirectory,
        applyCompression,
        sfpkgName,
        sfpkgFilePath);

    VERIFY(expectedError.ReadValue() == error.ReadValue(), "GenerateSfpkgWithValidation returned {0} {1}, expected {2}", error, error.Message, expectedError);

    if (error.IsSuccess())
    {
        VERIFY(File::Exists(sfpkgFilePath), "GenerateSfpkgWithValidation: File::Exists(sfpkgFilePath) returned false for {0}", sfpkgFilePath);
        VERIFY(Path::GetFileName(sfpkgFilePath) == expectedSfpkgName, "GenerateSfpkgWithValidation: file name {0} != expected {1}", sfpkgFilePath, expectedSfpkgName);
        VERIFY(Path::GetExtension(sfpkgFilePath) == L".sfpkg", "GenerateSfpkgWithValidation: file name {0} doesn't have sfpkg extension", sfpkgFilePath);

        // Destination folder exists and contains the sfpkg.
        VERIFY(Directory::Exists(destinationDirectory), "GenerateSfpkgWithValidation: destinationDirectory {0} should exist", destinationDirectory);
        VERIFY(Directory::GetFiles(destinationDirectory).size() >= 1, "GenerateSfpkgWithValidation: destinationDirectory {0} should have more than 1 files", destinationDirectory);

        VERIFY(Directory::Exists(appPackageRootDirectory), "GenerateSfpkgWithValidation: appPackageRootDirectory {0} should exist", appPackageRootDirectory);
    }
    else
    {
        VERIFY(!error.Message.empty(), "GenerateSfpkg returned error without message {0}", error);
    }
}

void ImageStoreUtilityTests::ExpandSfpkgWithValidation(
    wstring const & sfpkgFilePath,
    wstring const & destinationDirectory,
    bool sfpkgShouldBeDeleted,
    ErrorCode const & expectedError)
{
    auto error = ImageStoreUtility::ExpandSfpkg(sfpkgFilePath, destinationDirectory);

    VERIFY(expectedError.ReadValue() == error.ReadValue(), "ExpandSfpkgWithValidation returned {0} {1}, expected {2}", error, error.Message, expectedError);

    if (error.IsSuccess())
    {
        if (sfpkgShouldBeDeleted)
        {
            VERIFY(!File::Exists(sfpkgFilePath), "ExpandSfpkgWithValidation: file {0} should be deleted", sfpkgFilePath);
        }

        // The destination folder contains the expanded app package.
        VERIFY(Directory::Exists(destinationDirectory), "ExpandSfpkgWithValidation: destinationDirectory {0} should exist", destinationDirectory);
        VERIFY(Directory::GetFiles(destinationDirectory).size() >= 1, "ExpandSfpkgWithValidation: destinationDirectory {0} should have more than 1 files", destinationDirectory);
        VERIFY(Directory::GetSubDirectories(destinationDirectory).size() > 0, "ExpandSfpkgWithValidation: destinationDirectory {0} should have more than 1 directories", destinationDirectory);
    }
}

void ImageStoreUtilityTests::GenerateAppPackage(wstring const & appPackageFolder)
{
    CreateNewDirectory(appPackageFolder);

    wstring srcAppManifestFile(L"ImageStore.Test.ApplicationManifest.xml");
    wstring srcServiceManifestFile(L"ImageStore.Test.ServiceManifest.xml");

    // The service name is same as the name in ImageStore.Test.ServiceManifest.xml.
    wstring serviceManifestName(L"TestServices");

    wstring srcDirectory(Directory::GetCurrentDirectoryW());
    
    wstring bins;
    if (Environment::GetEnvironmentVariableW(L"_NTTREE", bins, Common::NOTHROW()))
    {
        srcDirectory = Path::Combine(bins, L"FabricUnitTests");
        Trace.WriteInfo(TestSource, L"GenerateAppPackage", "_NTTREE set, initialize srcDirectory={0}, appPackageFolder={1}", srcDirectory, appPackageFolder);
    }
    // RunCITs will end up setting current directory to FabricUnitTests\log
    else if (StringUtility::CompareCaseInsensitive(Path::GetFileName(srcDirectory), L"log") == 0)
    {
        // srcDirectory points to FabricUnitTests
        srcDirectory = Path::GetDirectoryName(srcDirectory);
        Trace.WriteInfo(TestSource, L"GenerateAppPackage", "_NTTREE NOT set, initialize srcDirectory={0}, appPackageFolder={1}", srcDirectory, appPackageFolder);
    }
    else
    {
        Trace.WriteWarning(TestSource, L"GenerateAppPackage", "_NTTREE NOT set, and directory doesn't end with 'log'. Initialize srcDirectory={0}, appPackageFolder={1}", srcDirectory, appPackageFolder);
    }
    
    BuildLayoutSpecification buildLayout(appPackageFolder);

    wstring appManifestPath = Path::Combine(srcDirectory, srcAppManifestFile);
    wstring appManifestDestPath = buildLayout.GetApplicationManifestFile();
    auto error = File::Copy(appManifestPath, appManifestDestPath);
    Trace.WriteInfo(TestSource, L"GenerateAppPackage", "Copy application manifest from {0} to {1} completed with {2}", appManifestPath, appManifestDestPath, error);

    VERIFY(error.IsSuccess(), "Copy app manifest from {0} to {1} failed with {2}", appManifestPath, appManifestDestPath, error.ErrorCodeValueToString());

    CreateNewDirectory(Path::Combine(appPackageFolder, serviceManifestName));

    error = File::Copy(
        Path::Combine(srcDirectory, srcServiceManifestFile),
        buildLayout.GetServiceManifestFile(serviceManifestName));
    VERIFY(error.IsSuccess(), "Copy service manifest {0} failed with {1}", srcServiceManifestFile, error.ErrorCodeValueToString());

    Directory::CreateDirectory(buildLayout.GetCodePackageFolder(serviceManifestName, L"TestService.Code"));
    File::Copy(
        Path::Combine(srcDirectory, srcServiceManifestFile),
        Path::Combine(buildLayout.GetCodePackageFolder(serviceManifestName, L"TestService.Code"), L"MyEntryPoint.exe"));

    Directory::CreateDirectory(buildLayout.GetConfigPackageFolder(serviceManifestName, L"TestService.Config"));
    Directory::CreateDirectory(buildLayout.GetDataPackageFolder(serviceManifestName, L"TestService.Data"));
}

void ImageStoreUtilityTests::CreateNewDirectory(wstring const & dirName)
{
    DeleteDirectory(dirName);

    auto error = Directory::Create2(dirName);
    VERIFY(error.IsSuccess(), "Create directory {0} completed with {1}", dirName, error);
}

void ImageStoreUtilityTests::DeleteDirectory(wstring const & dirName)
{
    if (!Directory::Exists(dirName))
    {
        return;
    }

    auto error = Directory::Delete(dirName, true);
    VERIFY(error.IsSuccess(), "Delete directory {0} failed", dirName);
}

bool ImageStoreUtilityTests::TestSetup()
{
    Trace.WriteNoise(TestSource, "++ Start test");
    // load tracing configuration
    Config cfg;
    Common::TraceProvider::LoadConfiguration(cfg);
    return true;
}

bool ImageStoreUtilityTests::TestCleanup()
{
    Trace.WriteNoise(TestSource, "++ Test Complete");
    return true;
}

