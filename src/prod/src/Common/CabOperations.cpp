// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#if !defined(PLATFORM_UNIX)
#pragma comment(lib,"cabinet.lib")
#endif

using namespace Common;

StringLiteral const TraceType = "CabOperations";

#if !defined(PLATFORM_UNIX)

// Abstract Base class for CAB operations
class CabOperations::CabOperation
{
    DENY_COPY(CabOperation)

public:
    CabOperation(std::wstring const & cabPath)
        : cabPath_(cabPath)
    {
        if (cabPath_.length() > MAX_PATH)
        {
            WriteError(TraceType, "Cab path too long.");
            error_ = FDIERROR_CABINET_NOT_FOUND;
            return;
        }

        fdiContext_ = FDICreate(
            fnMemAlloc,
            fnMemFree,
            fnFileOpen,
            fnFileRead,
            fnFileWrite,
            fnFileClose,
            fnFileSeek,
            cpuUNKNOWN,
            &fdiError_);

        if (fdiContext_ == NULL)
        {
            WriteError(TraceType, "FDICreate returned null handle. Error code {0}", fdiError_.erfOper);
            error_ = fdiError_.erfOper;
        }
        else
        {
            fileDescriptor_ = fnFileOpenW(cabPath_.c_str(), _O_BINARY | _O_RDONLY | _O_SEQUENTIAL, _S_IREAD);
            if (fileDescriptor_ <= 0)
            {
                WriteError(TraceType, "File descriptor on CAB file returned invalid value {0}.", fileDescriptor_);
                error_ = FDIERROR_CABINET_NOT_FOUND;
                return;
            }
            else
            {
                FDICABINETINFO cabInfo;
                if (!FDIIsCabinet(fdiContext_, fileDescriptor_, &cabInfo))
                {
                    WriteError(TraceType, "FDIIsCabinet determined {0} was not cab. Error code {1}", cabPath_.c_str(), fdiError_.erfOper);
                    error_ = FDIERROR_NOT_A_CABINET;
                }
            }
        }
    }

    ~CabOperation()
    {
        if (!FDIDestroy(fdiContext_))
        {
            WriteError(TraceType, "FDIDestroy failed with error - {0} : {1}", fdiError_.erfOper, FDIErrorToString((FDIERROR)fdiError_.erfOper));
        }

        if (fileDescriptor_ > 0 && fnFileClose(fileDescriptor_) != 0)
        {
            WriteError(TraceType, "fnFileClose failed with error code {0}", ::GetLastError());
        }
    }

    virtual int Execute() = 0;

    __declspec(property(get = GetIsValid)) bool IsValid;
    bool GetIsValid() { return error_ == FDIERROR_NONE; }

    __declspec(property(get = GetError)) int Error;
    int GetError() { return error_; }

protected:

#pragma region RequiredMethods
    virtual INT_PTR vCOPY_FILE(FDINOTIFICATIONTYPE fdint, PFDINOTIFICATION pfdin) = 0;

    // Derived from MSDN example: https://msdn.microsoft.com/en-us/library/ff797944(v=vs.85).aspx
    static FNFDINOTIFY(fnNotify)
    {
        INT_PTR result = 0;

        switch (fdint)
        {
        case fdintCOPY_FILE:
        {
            CabOperation* op = static_cast<CabOperation*>(pfdin->pv);
            result = op->vCOPY_FILE(fdint, pfdin);
            break;
        }
        case fdintCLOSE_FILE_INFO:
        {
            FILETIME fileTime;
            FILETIME fileTimeLocal;
            FILE_BASIC_INFO fbi;

            // Converts MS-DOS date and time values to a file time
            if (DosDateTimeToFileTime(pfdin->date, pfdin->time, &fileTime) == TRUE &&
                LocalFileTimeToFileTime(&fileTime, &fileTimeLocal) == TRUE)
            {
                fbi.CreationTime.LowPart = fileTimeLocal.dwLowDateTime;
                fbi.CreationTime.HighPart = fileTimeLocal.dwHighDateTime;
                fbi.LastWriteTime.QuadPart = -1;
                fbi.ChangeTime.QuadPart = -1;
                fbi.LastAccessTime.QuadPart = -1;

                // Retrieve the file attributes.
                fbi.FileAttributes = pfdin->attribs;

                // Set the date, time and attributes
                SetFileInformationByHandle((HANDLE)pfdin->hf,
                    FileBasicInfo,
                    &fbi,
                    sizeof(FILE_BASIC_INFO));
            }

            result = !fnFileClose(pfdin->hf);
            break;
        }
        case fdintNEXT_CABINET:
            if (pfdin->fdie != FDIERROR_NONE)
            {
                WriteError(TraceType, "Failed to process the spanned cabinet file \"{0}\" with error code {1}: {2}",
                    pfdin->psz1,
                    (int)pfdin->fdie,
                    FDIErrorToString(pfdin->fdie));
                result = -1;
            }
            break;
        case fdintPARTIAL_FILE:
            result = 0;
            break;
        case fdintCABINET_INFO:
            result = 0;
            break;
        case fdintENUMERATE:
            result = 0;
            break;
        default:
            result = -1;
            break;
        }

        return result;
    }

    static LPCSTR FDIErrorToString(FDIERROR err)
    {
        switch (err)
        {
        case FDIERROR_NONE:
            return "No error";

        case FDIERROR_CABINET_NOT_FOUND:
            return "Cabinet not found";

        case FDIERROR_NOT_A_CABINET:
            return "Not a cabinet";

        case FDIERROR_UNKNOWN_CABINET_VERSION:
            return "Unknown cabinet version";

        case FDIERROR_CORRUPT_CABINET:
            return "Corrupt cabinet";

        case FDIERROR_ALLOC_FAIL:
            return "Memory allocation failed";

        case FDIERROR_BAD_COMPR_TYPE:
            return "Unknown compression type";

        case FDIERROR_MDI_FAIL:
            return "Failure decompressing data";

        case FDIERROR_TARGET_FILE:
            return "Failure writing to target file";

        case FDIERROR_RESERVE_MISMATCH:
            return "Cabinets in set have different RESERVE sizes";

        case FDIERROR_WRONG_CABINET:
            return "Cabinet returned on fdintNEXT_CABINET is incorrect";

        case FDIERROR_USER_ABORT:
            return "Application aborted";

        default:
            return "Unknown error";
        }
    }

    // Using MSDN Default: https://msdn.microsoft.com/en-us/library/ff797929(v=vs.85).aspx
    static FNALLOC(fnMemAlloc)
    {
        return malloc(cb);
    }

    // Using MSDN Default: https://msdn.microsoft.com/en-us/library/ff797945(v=vs.85).aspx
    static FNFREE(fnMemFree)
    {
        free(pv);
    }

    // Derived from MSDN Default: https://msdn.microsoft.com/en-us/library/ff797946(v=vs.85).aspx
    static FNOPEN(fnFileOpen)
    {
        size_t len = strlen(pszFile) + 1;
        std::vector<WCHAR> u16_fileName(len);
        MultiByteToWideChar(CP_UTF8, 0, pszFile, -1, u16_fileName.data(), static_cast<int>(len));
        INT_PTR handle = fnFileOpenW(u16_fileName.data(), oflag, pmode);
        return handle;
    }

    static INT_PTR fnFileOpenW(const WCHAR* u16_fileName, int oflag, int)
    {
        HANDLE hFile = NULL;
        DWORD dwDesiredAccess = 0;
        DWORD dwCreationDisposition = 0;

        if (oflag & _O_RDWR)
        {
            dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
        }
        else if (oflag & _O_WRONLY)
        {
            dwDesiredAccess = GENERIC_WRITE;
        }
        else
        {
            dwDesiredAccess = GENERIC_READ;
        }

        if (oflag & _O_CREAT)
        {
            dwCreationDisposition = CREATE_ALWAYS;
        }
        else
        {
            dwCreationDisposition = OPEN_EXISTING;
        }

        hFile = CreateFileW(u16_fileName,
            dwDesiredAccess,
            FILE_SHARE_READ,
            NULL,
            dwCreationDisposition,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        return (INT_PTR)hFile;
    }

    // Using MSDN Default: https://msdn.microsoft.com/en-us/library/ff797930(v=vs.85).aspx
    static FNCLOSE(fnFileClose)
    {
        return (CloseHandle((HANDLE)hf) == TRUE) ? 0 : -1;
    }

    // Using MSDN Default: https://msdn.microsoft.com/en-us/library/ff797947(v=vs.85).aspx
    static FNREAD(fnFileRead)
    {
        DWORD dwBytesRead = 0;

        if (ReadFile((HANDLE)hf, pv, cb, &dwBytesRead, NULL) == FALSE)
        {
            dwBytesRead = (DWORD)-1L;
        }

        return dwBytesRead;
    }

    // Using MSDN Default: https://msdn.microsoft.com/en-us/library/ff797948(v=vs.85).aspx
    static FNSEEK(fnFileSeek)
    {
        return SetFilePointer((HANDLE)hf, dist, NULL, seektype);
    }

    // Using MSDN Default: https://msdn.microsoft.com/en-us/library/ff797949(v=vs.85).aspx
    static FNWRITE(fnFileWrite)
    {
        DWORD dwBytesWritten = 0;

        if (WriteFile((HANDLE)hf, pv, cb, &dwBytesWritten, NULL) == FALSE)
        {
            dwBytesWritten = (DWORD)-1;
        }

        return dwBytesWritten;
    }
#pragma endregion

protected:
    std::wstring cabPath_;
    HFDI fdiContext_ = NULL;
    INT_PTR fileDescriptor_ = NULL;
    int error_ = FDIERROR_NONE;
    ERF fdiError_;
};

class CabOperations::CabIsCabOperation : public CabOperation
{
    DENY_COPY(CabIsCabOperation)

public:
    CabIsCabOperation(std::wstring const & cabPath)
        : CabOperation(cabPath)
    {
    }

    virtual int Execute()
    {
        return FDIERROR_NONE;
    }

    virtual INT_PTR vCOPY_FILE(FDINOTIFICATIONTYPE, PFDINOTIFICATION)
    {
        return 0;
    }
};

class CabOperations::CabExtractAllOperation : public CabOperation
{
    DENY_COPY(CabExtractAllOperation)

public:
    CabExtractAllOperation(std::wstring const & cabPath, std::wstring const & extractPath)
        : CabOperation(cabPath)
        , destPath_(extractPath)
    {
        if (destPath_.length() > MAX_PATH)
        {
            WriteError(TraceType, "Cab extract path too long.");
            error_ = FDIERROR_CABINET_NOT_FOUND;
        }
    }

    virtual int Execute()
    {
        int result = FDIERROR_NONE;
        auto error = Directory::Create2(destPath_);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Destination directory could not be determined/created: {0}, Error:{1}", destPath_, error);
            return -1;
        }

        std::string cabFileName = formatString("{0}", Path::GetFileName(cabPath_));
        std::string cabDirectory = formatString("{0}\\", Path::GetDirectoryName(cabPath_));

        // Extract https://msdn.microsoft.com/en-us/library/ff797927(v=vs.85).aspx
        WriteInfo(TraceType, "CabExtract FDICopy using cab:{0}, dir:{1}", cabFileName, cabDirectory);
        if (!FDICopy(fdiContext_, (LPSTR)cabFileName.c_str(), (LPSTR) cabDirectory.c_str(), 0, fnNotify, 0, this))
        {
            WriteError(TraceType, "CabExtract FDICopy failed with {0} : {1}\n", fdiError_.erfOper, FDIErrorToString((FDIERROR)fdiError_.erfOper));
            result = fdiError_.erfOper;
        }

        return result;
    }

    virtual INT_PTR vCOPY_FILE(FDINOTIFICATIONTYPE, PFDINOTIFICATION pfdin)
    {
        std::wstring cabFilePath = wformatString("{0}", pfdin->psz1);

        if (ExcludeFile(cabFilePath))
        {
            return 0;
        }

        // Check length of dir+filename does not surpass MAX_PATH else break with error
        if (cabFilePath.length() + destPath_.length() > MAX_PATH)
        {
            WriteError(TraceType, "FNFDINOTIFY fdintCOPY_FILE failed due to long path extracting {0}", pfdin->psz1);
            return -1;
        }

        auto fullPathRequired = Path::Combine(destPath_, Path::GetDirectoryName(cabFilePath));
        ErrorCode error = Directory::Create2(fullPathRequired);
        if (!error.IsSuccess())
        {
            WriteError(TraceType, "Unable to create directory {0} because {1}", fullPathRequired, error);
            return -1;
        }

        std::wstring fullFileName = Path::Combine(destPath_, cabFilePath);
        WriteInfo(TraceType, "Extract File \"{0}\" (Size: {1} bytes) -> \"{2}\"", cabFilePath, pfdin->cb, fullFileName);
        INT_PTR result = fnFileOpenW(fullFileName.c_str(), _O_WRONLY | _O_CREAT, 0);
        if (result <= 0)
        {
            WriteError(TraceType, "FNFDINOTIFY fdintCOPY_FILE fnFileOpenW failed with {0} for file:{1}", result, fullFileName);
            if (result == 0) result = -1;
        }

        return result;
    }

protected:
    virtual bool ExcludeFile(std::wstring const &)
    {
        return false;
    }

    std::wstring destPath_;
};

class CabOperations::CabExtractFilteredOperation : public CabExtractAllOperation
{
    DENY_COPY(CabExtractFilteredOperation)

public:
    CabExtractFilteredOperation(std::wstring const & cabPath, std::wstring const & extractPath, std::vector<std::wstring> const & filters, bool inclusive)
        : CabExtractAllOperation(cabPath, extractPath)
        , filters_(filters)
        , inclusive_(inclusive)
    {}

    bool ExcludeFile(std::wstring const & fileName)
    {
        bool found = false;
        for (auto i = 0; i < filters_.size() && !found; i++)
        {
            found = StringUtility::StartsWith(fileName, filters_[i]);
        }
        return inclusive_ && !found || found && !inclusive_;
    }

private:
    std::vector<std::wstring> filters_;
    bool inclusive_;
};

class CabOperations::CabContainsFileOperation : public CabOperation
{
    DENY_COPY(CabContainsFileOperation)

public:
    CabContainsFileOperation(std::wstring const & cabPath, std::wstring const & checkedFile)
        : CabOperation(cabPath)
        , inputFileName_(checkedFile)
        , fileFound_(false)
    {
    }

    virtual int Execute()
    {
        std::string cabFileName = formatString("{0}", Path::GetFileName(cabPath_));
        std::string cabDirectory = formatString("{0}\\", Path::GetDirectoryName(cabPath_));

        // Extract https://msdn.microsoft.com/en-us/library/ff797927(v=vs.85).aspx
        WriteInfo(TraceType, "CabContains FDICopy using cab:{0}, dir:{1}", cabFileName.c_str(), cabDirectory.c_str());
        if (!FDICopy(fdiContext_, (LPSTR)cabFileName.c_str(), (LPSTR)cabDirectory.c_str(), 0, fnNotify, 0, this))
        {
            // If fileFound_ is true, error condition was forced to short-circuit the search
            if (!fileFound_)
            {
                WriteError(TraceType, "CabContains FDICopy failed with {0} : {1}", fdiError_.erfOper, FDIErrorToString((FDIERROR)fdiError_.erfOper));
                error_ = fdiError_.erfOper;
            }
        }

        return fileFound_ ? TRUE : FALSE;
    }

    virtual INT_PTR vCOPY_FILE(FDINOTIFICATIONTYPE, PFDINOTIFICATION pfdin)
    {
        std::wstring filePath = wformatString("{0}", pfdin->psz1);
        std::wstring fileName = Path::GetFileName(filePath);
        if (Common::StringUtility::Compare(fileName, inputFileName_) == 0)
        {
            fileFound_ = true;
            return -1;
        }

        return NULL;
    }

private:
    std::wstring inputFileName_;
    bool fileFound_;
};
#endif //!defined(PLATFORM_UNIX)

// ----------------------------------------------------------------------------
// CabOperations Definitions
// ----------------------------------------------------------------------------
bool CabOperations::IsCabFile(std::wstring const & cabPath)
{
#if !defined(PLATFORM_UNIX)
    auto op = make_unique<CabIsCabOperation>(cabPath);
    return op->IsValid;
#else
    // Not Implemented
    return -1;
#endif
}

int CabOperations::ExtractAll(std::wstring const & cabPath, std::wstring const & extractPath)
{
#if !defined(PLATFORM_UNIX)
    auto op = make_unique<CabExtractAllOperation>(cabPath, extractPath);
    return op->IsValid ? op->Execute() : op->Error;
#else
    // Not Implemented
    return -1;
#endif
}

int CabOperations::ExtractFiltered(std::wstring const & cabPath, std::wstring const & extractPath, std::vector<std::wstring> const & filters, bool inclusive)
{
#if !defined(PLATFORM_UNIX)
    auto op = make_unique<CabExtractFilteredOperation>(cabPath, extractPath, filters, inclusive);
    return op->IsValid ? op->Execute() : op->Error;
#else
    // Not Implemented
    return -1;
#endif
}

int CabOperations::ContainsFile(__in std::wstring const & cabPath, __in std::wstring const & checkedFile, __out bool & found)
{
#if !defined(PLATFORM_UNIX)
    auto op = make_unique<CabContainsFileOperation>(cabPath, checkedFile);
    if (op->IsValid)
    {
        found = op->Execute() == TRUE;
    }
    else
    {
        found = false;
    }
    return op->Error;
#else
    // Not Implemented
    return -1;
#endif
}
