// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace FabricInstallerService;

const StringLiteral TraceType("Utility");

struct Utility::FileInfo
{
    FileInfo()
        : version(0)
    {
    }

    FileInfo(wstring const & f, unsigned __int64 v)
        : filepath(f)
        , version(v)
    {
    }

    wstring filepath;
    unsigned __int64 version;
};

ErrorCode Utility::MoveFileAtReboot(std::wstring const & src, std::wstring const & dest)
{
    if (!::MoveFileEx(src.c_str(), dest.c_str(), 
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode Utility::DeleteFileAtReboot(std::wstring const & filepath)
{
    if (!::MoveFileEx(filepath.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode Utility::DeleteDirectoryAtReboot(std::wstring const & directoryPath)
{
    ErrorCode error(ErrorCodeValue::Success);

    // MoveFileEx with MOVEFILE_DELAY_UNTIL_REBOOT requires directories to be empty for deletion, 
    // so have to schedule removal of each recursive file/dir
    ASSERT_IF(directoryPath.empty(), "DeleteDirectoryAtReboot directoryPath cannot be empty.");

    // Schedule delete all files in bin\\*
    vector<wstring> files = Directory::GetFiles(directoryPath, Constants::FindAllSearchPattern, true, false);
    for (wstring const & file : files)
    {
        error = Utility::DeleteFileAtReboot(file);
        // Schedule move at reboot should never fail. Same for each call below.
        ASSERT_IFNOT(error.IsSuccess(), "DeleteDirectoryAtReboot OnReboot schedule file delete error {0} on file {1}.", error, file);
    }

    // Schedule delete of all existing directories in bin\\*
    vector<wstring> subDirs = Directory::GetSubDirectories(directoryPath, Constants::FindAllSearchPattern, true, false);
    std::reverse(subDirs.begin(), subDirs.end()); // Directories have to be deleted depth-first
    for (wstring const & dir : subDirs)
    {
        error = Utility::DeleteFileAtReboot(dir);
        ASSERT_IFNOT(error.IsSuccess(), "DeleteDirectoryAtReboot OnReboot schedule directory delete error {0} on directory {1}.", error, dir);
    }

    return error;
}

ErrorCode Utility::ReplaceDirectoryAtReboot(std::wstring const & srcDir, std::wstring const & destDir)
{
    const wstring MethodId = L"ReplaceDirectoryAtReboot";
    ErrorCode error(ErrorCodeValue::Success);

    ASSERT_IF(srcDir.empty(), "ReplaceDirectoryAtReboot srcDir cannot be empty.");
    ASSERT_IF(destDir.empty(), "ReplaceDirectoryAtReboot destDir cannot be empty.");

    wstring srcDirFormatted = srcDir[srcDir.length() - 1] == L'\\'
        ? srcDir
        : srcDir + L'\\';

    // Schedule delete of existing destination directory
    error = DeleteDirectoryAtReboot(destDir);
    ASSERT_IFNOT(error.IsSuccess(), "ReplaceDirectoryAtReboot failed deleting directory {0} with error: {1}", destDir, error);

    // Schedule move of directories, using temporary directories
    vector<wstring> sourceDirs = Directory::GetSubDirectories(srcDirFormatted, Constants::FindAllSearchPattern, true, false);
    vector<wstring> destRelativeDirs;
    wstring extrasBaseDir = Path::Combine(srcDirFormatted, Constants::ExtraDirsDirectoryName) + L'\\';
    if (Directory::Exists(extrasBaseDir)) // Normally shouldn't be present
    {
        WriteInfo(TraceType, MethodId, "Deleting UT\\x\\ED directory from previous run: {0}", extrasBaseDir);
        error = Directory::Delete(extrasBaseDir, true, true);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, MethodId, "Deleting UT\\x\\ED {0} dir hit error: {1}", extrasBaseDir, error);
            error.Reset();
        }
    }

    error = Directory::Create2(extrasBaseDir);
    ASSERT_IFNOT(error.IsSuccess(), "Creating UT\\x\\ED {0} dir hit error: {1}", extrasBaseDir, error);

    destRelativeDirs = sourceDirs;
    wstring newDir;
    wstring newDestDir;
    // Create flat arrangement of temporary empty dirs and schedule to move them with 
    // new name corresponding to intended directory names. This is done as a workaround 
    // to moving the existing dirs since there is a circular relationship to moving 
    // folders and the files they contain. Move on reboot doesn't work for directories 
    // containing anything, but files have to be moved after their directories, so 
    // duplicate directories and delete the original dirs.
    for (auto& rd : destRelativeDirs)
    {
        rd = rd.substr(srcDirFormatted.length());
        newDir = Path::Combine(extrasBaseDir, Guid::NewGuid().ToString());
        newDestDir = Path::Combine(destDir, rd);
        Directory::Create(newDir);
        WriteInfo(TraceType, MethodId, "Scheduling move: {0} to {1}", newDir, newDestDir);
        error = MoveFileAtReboot(newDir, newDestDir);
        ASSERT_IFNOT(error.IsSuccess(), "MoveFileAtReboot failed {0} to {1} with error: {2}", newDir, newDestDir, error);
    }

    WriteInfo(TraceType, MethodId, "Scheduling delete: {0}", extrasBaseDir);
    error = DeleteFileAtReboot(extrasBaseDir);
    ASSERT_IFNOT(error.IsSuccess(), "DeleteFileAtReboot failed {0} with error: {1}", extrasBaseDir, error);

    // Schedule move of files
    vector<wstring> sourceFiles = Directory::GetFiles(srcDirFormatted, Constants::FindAllSearchPattern, true, false);
    wstring relativeFilePath;
    wstring destinationFilePath;
    for (auto& file : sourceFiles)
    {
        relativeFilePath = file.substr(srcDirFormatted.length());
        destinationFilePath = Path::Combine(destDir, relativeFilePath);
        WriteInfo(TraceType, MethodId, "Scheduling move: {0} to {1}", file, destinationFilePath);
        error = MoveFileAtReboot(file, destinationFilePath);
        ASSERT_IFNOT(error.IsSuccess(), "MoveFileAtReboot failed {0} to {1} with error: {2}", file, destinationFilePath, error);
    }

    // Schedule delete of subdirectories - since we duplicated these as a workaround to circular dependency of folders having to be moved.
    std::reverse(sourceDirs.begin(), sourceDirs.end()); // Directories have to be deleted depth-first
    for (auto& dir : sourceDirs)
    {
        WriteInfo(TraceType, MethodId, "Scheduling delete: {0}", dir);
        error = DeleteFileAtReboot(dir);
        ASSERT_IFNOT(error.IsSuccess(), "DeleteFileAtReboot failed {0} error: {2}", dir, error);
    }

    WriteInfo(TraceType, MethodId, "Scheduling delete: {0}", srcDir);
    error = DeleteFileAtReboot(srcDir);
    ASSERT_IFNOT(error.IsSuccess(), "DeleteFileAtReboot failed {0} with error: {1}", srcDir, error);

    return error;
}

ErrorCode Utility::RebootMachine()
{
    const wstring MethodId = L"RebootMachine";
    HANDLE hToken = INVALID_HANDLE_VALUE;
    TOKEN_PRIVILEGES tkp;
    ErrorCode error(ErrorCodeValue::Success);

    // Get a token for this process. 
    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
    {
        error = ErrorCode::FromWin32Error(::GetLastError());
        WriteWarning(
            TraceType,
            MethodId,
            "Error {0} while opening process token",
            error);
        return error;
    }

    Handle handle(hToken);
    // Get the LUID for the shutdown privilege. 
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // Get the shutdown privilege for this process. 
    if (!AdjustTokenPrivileges(handle.Value, FALSE, &tkp, 0,
        (PTOKEN_PRIVILEGES)NULL, 0))
    {
        error = ErrorCode::FromWin32Error(::GetLastError());
        WriteWarning(
            TraceType,
            MethodId,
            "Error {0} while adjusting token privileges",
            error);
        return error;
    }

    // Shut down the system and force all applications to close. 
    if (!ExitWindowsEx(EWX_REBOOT | EWX_FORCE,
        SHTDN_REASON_MAJOR_APPLICATION |
        SHTDN_REASON_MINOR_UPGRADE))
    {
        error = ErrorCode::FromWin32Error(::GetLastError());
        WriteWarning(
            TraceType,
            MethodId,
            "Error {0} while shutting down machine",
            error);
        return error;
    }

    // Shutdown was successful
    if (INVALID_HANDLE_VALUE != hToken)
    {
        ::CloseHandle(hToken);
    }

    return ErrorCodeValue::Success;
}

ErrorCode Utility::WriteUpgradeValidationFile(wstring const & validationFilePath, vector<wstring> const & versionTargetFiles, wstring const & rootDirectory)
{
    ErrorCode error(ErrorCodeValue::Success);
    vector<FileInfo> files(versionTargetFiles.size());
    size_t rootDirLen = rootDirectory.size();

    // Get versions
    for (size_t i = 0; i < files.size(); i++)
    {
        // Strip root directory prefix
        files[i].filepath = versionTargetFiles[i].substr(rootDirLen);
        files[i].version = FileVersion::GetFileVersionNumber(versionTargetFiles[i], false);
    }

    error = WriteValidationFile(validationFilePath, files);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            L"WriteValidationFile",
            "Writing validation file {0} failed with error {1}",
            validationFilePath,
            error);
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode Utility::ValidateUpgrade(wstring const & validationFilePath, wstring const & rootDirectory)
{
    const wstring MethodId = L"ValidateUpgrade";
    vector<FileInfo> files;
    auto error = ReadUpgradeValidationFile(validationFilePath, files);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            MethodId,
            "Reading validation file {0} failed with error {1}",
            validationFilePath,
            error);
        return error;
    }

    wstring destinationFile;
    unsigned __int64 destinationVersion;
    bool success = true;
    for (FileInfo& fi : files)
    {
        destinationFile = Path::Combine(rootDirectory, fi.filepath);
        if (!File::Exists(destinationFile))
        {
            success = false;
            WriteError(
                TraceType,
                MethodId,
                "Validation missing file {0}.",
                destinationFile,
                error);
        }

        destinationVersion = FileVersion::GetFileVersionNumber(destinationFile, false);
        if (fi.version == destinationVersion)
        {
            WriteInfo(
                TraceType,
                MethodId,
                "Version match {0}: '{1}'",
                destinationFile,
                FileVersion::NumberVersionToString(destinationVersion));
        }
        else
        {
            success = false;
            WriteError(
                TraceType,
                MethodId,
                "Version mismatch {0}: '{1}' supposed to be '{2}'",
                destinationFile,
                FileVersion::NumberVersionToString(destinationVersion),
                FileVersion::NumberVersionToString(fi.version));
        }
    }

    if (!success)
    {
        return ErrorCodeValue::OperationFailed;
    }

    return ErrorCodeValue::Success;
}

ErrorCode Utility::WriteValidationFile(wstring const & outFilePath, const vector<FileInfo> & files)
{
    const wstring MethodId = L"WriteUpgradeValidationFile";
    vector<unsigned char> fileContents;
    size_t contentSize = 0;
    size_t contentCapacity = Constants::EstimatedValidationFileSize;
    fileContents.resize(contentCapacity);

    size_t pathLenInBytes;
    size_t fileInfoLen;
    size_t newContentSize;

    // Memory:
    // [size_t number of files][wrelative path filename \0\0][unsigned __int64]...
    // [wrelative path filename \0\0][unsigned __int64]...
    // [wrelative path filename \0\0][unsigned __int64]\n

    unsigned char* writePtr = fileContents.data();
    // Write number of files as size_t
    newContentSize = sizeof(size_t);
    if (newContentSize > contentCapacity)
    {
        fileContents.resize(newContentSize + 1); // For EOF marker
        writePtr = fileContents.data() + contentSize; // reset writePtr to new location after resize
        contentCapacity = newContentSize;
    }
    contentSize = newContentSize;
    *(size_t*)writePtr = (size_t)files.size(); writePtr += sizeof(size_t);

    // Write File contents
    for (const FileInfo& fi : files)
    {
        // Calculate sizes
        const wchar_t* path = fi.filepath.c_str();
        pathLenInBytes = (wcslen(path) + 1)*sizeof(wchar_t); // suffix 2 bytes of \0
        fileInfoLen = pathLenInBytes + sizeof(__int64);
        newContentSize = contentSize + fileInfoLen;
        if (newContentSize > contentCapacity)
        {
            fileContents.resize(newContentSize + 1); // For EOF marker
            writePtr = fileContents.data() + contentSize; // reset writePtr to new location after resize
            contentCapacity = newContentSize;
        }
        contentSize = newContentSize;

        // Write memory
        memcpy((void*)writePtr, (void*)path, pathLenInBytes);
        writePtr += pathLenInBytes;
        *(unsigned __int64*)writePtr = fi.version;
        writePtr += sizeof(__int64);
    }
    contentSize++;
    *writePtr = '\n'; // EOF marker

    File outFile;
    auto error = outFile.TryOpen(outFilePath, FileMode::Create, FileAccess::Write, FileShare::None);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            MethodId,
            "Unable to open output file {0}. Error {1}",
            outFilePath,
            error);
        return error;
    }

    DWORD bytesWritten;
    error = outFile.TryWrite2(fileContents.data(), (int)contentSize, bytesWritten);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            MethodId,
            "Unable to write output file {0}. Bytes: {1}. Error {2}",
            outFilePath,
            bytesWritten,
            error);
        return error;
    }

    return ErrorCodeValue::Success;
}

ErrorCode Utility::ReadUpgradeValidationFile(wstring const & readFilePath, vector<FileInfo> & resultFiles)
{
    const wstring MethodId = L"ReadUpgradeValidationFile";
    File inFile;
    auto error = inFile.TryOpen(readFilePath, FileMode::Open, FileAccess::Read);
    if (!error.IsSuccess())
    {
        WriteError(
            TraceType,
            L"ReadUpgradeValidationFile",
            "Unable to open input file {0}. Error {1}",
            readFilePath,
            error);
        return error;
    }

    int fileSize = static_cast<int>(inFile.size());
    vector<unsigned char> fileContents(fileSize);

    if (!inFile.TryRead(reinterpret_cast<void*>(fileContents.data()), fileSize))
    {
        WriteError(
            TraceType,
            MethodId,
            "Failed to reach from input file {0}.",
            readFilePath);
        return ErrorCodeValue::OperationFailed;
    }

    if (fileSize < (sizeof(size_t)+sizeof(unsigned char))) // Num files + EOF
    {
        WriteError(
            TraceType,
            MethodId,
            "Upgrade validation file invalid: {0}.",
            readFilePath);
        return ErrorCodeValue::InvalidArgument;
    }

    size_t noFiles;
    size_t pathLenInBytes;
    wchar_t* nextPath;
    unsigned __int64 nextVersion;

    unsigned char* readPtr = fileContents.data();
    // Extract no. of files
    noFiles = *(size_t*)readPtr; readPtr += sizeof(size_t);
    resultFiles.resize(noFiles);

    // While input is valid & not EOF
    for (size_t i = 0; i < noFiles && *readPtr != NULL && *readPtr != '\n'; i++)
    {
        nextPath = (wchar_t*)readPtr;
        pathLenInBytes = (wcslen(nextPath) + 1)*sizeof(wchar_t);
        readPtr += pathLenInBytes;
        nextVersion = *(unsigned __int64*)readPtr; 
        readPtr += sizeof(__int64);

        resultFiles[i].filepath = nextPath;
        resultFiles[i].version = nextVersion;
    }

    return ErrorCodeValue::Success;
}
