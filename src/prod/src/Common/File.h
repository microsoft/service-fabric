// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "FabricTypes_.h"

#if defined(PLATFORM_UNIX)
extern DWORD FileGetLastErrorFromErrno(void);
extern DWORD DirGetLastErrorFromErrno();
extern std::string FileNormalizePath(LPCWSTR pathW);
extern bool GlobMatch(const char *str, const char *pat);
#endif

namespace Common
{
    class ErrorCode;

    namespace SeekOrigin
    {
        enum Enum
        {
            Begin = FILE_BEGIN,
            Current = FILE_CURRENT,
            End = FILE_END,
        };
    };

    namespace FileMode {
        enum Enum {
    // UNDONE:  Append,
            Create = CREATE_ALWAYS,
            CreateNew = CREATE_NEW,
            Open = OPEN_EXISTING,
            OpenOrCreate = OPEN_ALWAYS,
            Truncate = TRUNCATE_EXISTING
        };
    };

    namespace FileAccess {
        enum Enum {
            None = 0,
            Read = GENERIC_READ,
            ReadAttributes = FILE_READ_ATTRIBUTES,
            ReadWrite = GENERIC_READ | GENERIC_WRITE,
            Write = GENERIC_WRITE,
            Delete = DELETE
        };
    };

    namespace MoveFileMode {
        enum Enum {
            ReplaceExisting = MOVEFILE_REPLACE_EXISTING,
            WriteThrough = MOVEFILE_WRITE_THROUGH
        };
    };

    namespace FileAttributes {
        enum Enum
        {
            None = 0,

            ReadOnly = FILE_ATTRIBUTE_READONLY,
            Hidden = FILE_ATTRIBUTE_HIDDEN,
            System = FILE_ATTRIBUTE_SYSTEM,
            Directory = FILE_ATTRIBUTE_DIRECTORY,
            Archive = FILE_ATTRIBUTE_ARCHIVE,
            Device = FILE_ATTRIBUTE_DEVICE,
            Normal = FILE_ATTRIBUTE_NORMAL,
            Temporary = FILE_ATTRIBUTE_TEMPORARY,
            SparseFile = FILE_ATTRIBUTE_SPARSE_FILE,
            ReparsePoint = FILE_ATTRIBUTE_REPARSE_POINT,
            Compressed = FILE_ATTRIBUTE_COMPRESSED,
            Offline = FILE_ATTRIBUTE_OFFLINE,
            NotContentIndexed = FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
            Encrypted = FILE_ATTRIBUTE_ENCRYPTED,
            FirstPipeInstance = FILE_FLAG_FIRST_PIPE_INSTANCE,
            OpenNoRecall = FILE_FLAG_OPEN_NO_RECALL,
            OpenReparsePoint = FILE_FLAG_OPEN_REPARSE_POINT,
            SessionAware = FILE_FLAG_SESSION_AWARE,
            PosixSemantics = FILE_FLAG_POSIX_SEMANTICS,
            BackupSemantics = FILE_FLAG_BACKUP_SEMANTICS,
            DeleteOnClose = FILE_FLAG_DELETE_ON_CLOSE,
            SequentialScan = FILE_FLAG_SEQUENTIAL_SCAN,
            RandomAccess = FILE_FLAG_RANDOM_ACCESS,
            NoBuffering = FILE_FLAG_NO_BUFFERING,
            Overlapped = FILE_FLAG_OVERLAPPED,
            WriteThrough = FILE_FLAG_WRITE_THROUGH,
        };
    };

    namespace FileShare {
        enum Enum
        {
            None = 0,
            Read = FILE_SHARE_READ,
        
            // Allows subsequent opening of the file for writing. If this flag is not
            // specified, any request to open the file for writing (by this process or
            // another process) will fail until the file is closed.
            /// <include file='doc\FileShare.uex' path='docs/doc[@for="FileShare.Write"]/*' />
            Write = FILE_SHARE_WRITE,
        
            // Allows subsequent opening of the file for writing or reading. If this flag
            // is not specified, any request to open the file for writing or reading (by
            // this process or another process) will fail until the file is closed.
            ReadWrite = FILE_SHARE_READ | FILE_SHARE_WRITE,

            // Open the file, but allow someone else to delete the file.
            Delete = FILE_SHARE_DELETE,

            ReadWriteDelete = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        };
    };

    class File
    {
        File(const File&) = delete;
        File& operator = (const File&) = delete;

    public:

        File();

        File(File &&);

        explicit File(::HANDLE handle);

        File& operator = (File &&);

        ErrorCode Open(
            std::wstring const & fname, 
            FABRIC_FILE_MODE fileMode, 
            FABRIC_FILE_ACCESS fileAccess, 
            FABRIC_FILE_SHARE fileShare,
            ::HANDLE * fileHandle);

        ErrorCode Open(
            std::wstring const & fname,
            FABRIC_FILE_MODE fileMode,
            FABRIC_FILE_ACCESS fileAccess,
            FABRIC_FILE_SHARE fileShare,
            FABRIC_FILE_ATTRIBUTES fileAttributes,
            ::HANDLE * fileHandle);

        ErrorCode TryOpen(
            std::wstring const & fname, 
            FileMode::Enum mode = FileMode::Open, 
            FileAccess::Enum access = FileAccess::Read, 
            FileShare::Enum share = FileShare::None,
            FileAttributes::Enum attributes = FileAttributes::None);

        ErrorCode File::TryRead2(__out_bcount_part(count, bytesRead) void* buffer, int count, DWORD &bytesRead);
        ErrorCode File::TryWrite2(__in_bcount(count) const void* buffer, int count, DWORD & bytesWritten);

        __declspec(property(get=get_FileName)) std::wstring const & FileName;
        std::wstring const & get_FileName() const { return fileName_; }

        void Close();
        ErrorCode Close2();
        void GetWriteTime(FILETIME & fileWriteTime);
        virtual int TryRead(__out_bcount_part(count, return) void* buffer, int count); 
        virtual int TryWrite(__in_bcount(count) const void* buffer, int count);        

        void Read(__out_bcount_full(count) void* buffer, int count)
        {
            int read = TryRead(buffer, count);
            if (read != count)
                THROW(WinError(GetLastError()), "unexpected number of bytes read");
        }

        void Write(__in_bcount(count) const void* buffer, int count)
        {
            int written = TryWrite(buffer, count);
            if (written != count)
                THROW(WinError(GetLastError()), "unexpected number of bytes written");
        }

        bool IsValid() const 
        {
            return INVALID_HANDLE_VALUE != handle_;
        }

        virtual int64 size() const; // todo, check if this needs to be virtual
        virtual bool TryGetSize(__out int64 & size) const; // todo, check if this needs to be virtual
        virtual ErrorCode GetSize(_Out_ int64 & size) const;
        static ErrorCode GetSize(std::wstring const &, _Out_ int64 & size);
#if !defined(PLATFORM_UNIX)
        virtual void resize(int64 position);
#endif

        int64 Seek(int64 offset, Common::SeekOrigin::Enum origin);

        static ErrorCode GetAttributes(const std::wstring& path, FileAttributes::Enum & attributes);
        static ErrorCode SetAttributes(const std::wstring& path, FileAttributes::Enum fileAttributes);
        static ErrorCode RemoveReadOnlyAttribute(const std::wstring& path);
#if defined(PLATFORM_UNIX)
        static ErrorCode AllowAccessToAll(const std::wstring& path);
#endif

        void Flush();

        ~File()
        {
            //DTOR_BEGIN
            if (isHandleOwned_)
            {
                Close();
            }
            //DTOR_END
        }

        int ReadByte() 
        {
            byte b;
            ::DWORD bytesRead;
            CHK_WBOOL(::ReadFile(handle_, &b, 1, &bytesRead, nullptr));
            if (bytesRead == 1)
                return b;
            else 
                return -1;
        }

        // The following three functions are used by ut only and 
        // does not seem to belong to a File class at all
        bool StringAtPosition(std::wstring const & value, int atPosition);
        bool StartsWith(std::wstring const & value) { return StringAtPosition(value, 0); }
        bool Contains(std::wstring const & value);

        //// SIN: refactor until you don't need this method
        // ISSUE:2006/07/25-jonfisch  I needed this for MiniDumpWriteDump and didn't feel like 
        // writing wrappers, so sue me :).
        ::HANDLE GetHandle() { return handle_; }

        static std::wstring GetTempFileName(std::wstring const& path = L"");

        static ErrorCode Copy(
            const std::wstring& sourceFileName,
            const std::wstring& destFileName,
            bool overwrite = false);

#if defined(PLATFORM_UNIX)
        static ErrorCode Copy(
                const std::wstring& sourceFileName,
                const std::wstring& destFileName,
                const std::wstring& accountName,
                const std::wstring& password,
                bool overwrite = false);
#endif

        static ErrorCode SafeCopy(
            const std::wstring& sourceFileName,
            const std::wstring& destFileName,
            bool overWrite = false,
            bool shouldAcquireLock = true,
            Common::TimeSpan const timeout = TimeSpan::Zero);

        ErrorCode Delete();

        static void Delete( std::wstring const & path, bool throwIfNotFound = true );

        static bool Delete(std::wstring const & path, NOTHROW, bool const deleteReadonly = false);

        static ErrorCode Delete2( std::wstring const & path, bool const deleteReadonly = false);

        static ErrorCode Replace(std::wstring const & replacedFileName, std::wstring const & replacementFileName, std::wstring const & backupFileName, bool ignoreMergeErrors);

        static ErrorCode Move( const std::wstring& SourceFile, const std::wstring& DestFilei, bool throwIfFail = true);

        static ErrorCode MoveTransacted(const std::wstring & src, const std::wstring & dest, bool overwrite);

        static ErrorCode Echo(const std::wstring & src, const std::wstring & dest, Common::TimeSpan const timeout = TimeSpan::Zero);

        static ErrorCode GetLastWriteTime(std::wstring const & path, __out DateTime & lastWriteTime);

        static bool Exists(const std::wstring& path);

        static bool CreateHardLink(std::wstring const & fileName, std::wstring const & existingFileName);

#if !defined(PLATFORM_UNIX)
        static ErrorCode FlushVolume(const WCHAR driveLetter);
#endif

        static ErrorCode Touch(std::wstring const & fileName);

        static std::string GetFileAccessDiagnostics(std::string const & path);

    public:

        class IFileEnumerator
        {
        public:
            virtual ErrorCode MoveNext() = 0;
            virtual WIN32_FIND_DATA const & GetCurrent() = 0;
            virtual DWORD GetCurrentAttributes() = 0;
            virtual std::wstring GetCurrentPath() = 0;
        };

        static std::shared_ptr<IFileEnumerator> Search(std::wstring && pattern);

    private:
        class FileFind;

        std::wstring fileName_;
        ::HANDLE handle_;
        bool isHandleOwned_;
    };

    ///
    /// A class that creates a handle to a file at the provided location and marks the file for deletion.  Upon close, this object will
    /// delete the created file.
    ///
    class temporary_file
    {
        DENY_COPY(temporary_file);
    public:
        explicit temporary_file(std::wstring const & fileName);
        ~temporary_file();

    private:
        HANDLE file_handle_;
        std::wstring const & file_name_;
    };

} // end namespace Common
