// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType_XmlFileStream = "XmlFileStream";

ComXmlFileStream::ComXmlFileStream(HANDLE hFile) 
    : hFile_(hFile)
{
    hFile_ = hFile;
}

ComXmlFileStream::~ComXmlFileStream()
{
    if (hFile_ != INVALID_HANDLE_VALUE)
    {
#if !defined(PLATFORM_UNIX)
        ::CloseHandle(hFile_);
#else
        close((int)hFile_);
#endif
    }
}

Common::ErrorCode ComXmlFileStream::Open(
    std::wstring fileName, 
    bool fWrite, 
    __out ComPointer<IStream> & xmlFileStream)
{
    HANDLE hFile = ::CreateFileW(
        fileName.c_str(), 
        fWrite ? GENERIC_WRITE : GENERIC_READ, 
        FILE_SHARE_READ,
        NULL, 
        fWrite ? CREATE_ALWAYS : OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        WriteWarning(
            TraceType_XmlFileStream,
            "CreateFileW failed: file={0} error={1}",
            fileName,
            ::GetLastError());

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    try
    {
        xmlFileStream = make_com<ComXmlFileStream, IStream>(hFile);
    }
    catch(...)
    {
#if !defined(PLATFORM_UNIX)
        ::CloseHandle(hFile);
#else
        close((int)hFile);
#endif
        throw;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

HRESULT STDMETHODCALLTYPE ComXmlFileStream::Read(void* pv, ULONG cb, ULONG* pcbRead)
{
    BOOL rc = ::ReadFile(hFile_, pv, cb, pcbRead, NULL);
    if (rc)
    {
        return ComUtility::OnPublicApiReturn(S_OK);
    }
    else
    {
        auto error = GetLastError();
        WriteWarning(
            TraceType_XmlFileStream,
            "ReadFile failed {0}",
            error);

        return ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(error));
    }
}

HRESULT STDMETHODCALLTYPE ComXmlFileStream::Write(void const* pv, ULONG cb, ULONG* pcbWritten)
{
    BOOL rc = WriteFile(hFile_, pv, cb, pcbWritten, NULL);
    if (rc)
    {
        return ComUtility::OnPublicApiReturn(S_OK);
    }
    else
    {
        auto error = GetLastError();
        WriteWarning(
            TraceType_XmlFileStream,
            "WriteFile failed {0}",
            error);

        return ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(error));
    }
}

HRESULT STDMETHODCALLTYPE ComXmlFileStream::Seek(
    LARGE_INTEGER liDistanceToMove, 
    DWORD dwOrigin,
    ULARGE_INTEGER* lpNewFilePointer)
{ 
    DWORD dwMoveMethod;

    switch(dwOrigin)
    {
    case STREAM_SEEK_SET:
        dwMoveMethod = FILE_BEGIN;
        break;
    case STREAM_SEEK_CUR:
        dwMoveMethod = FILE_CURRENT;
        break;
    case STREAM_SEEK_END:
        dwMoveMethod = FILE_END;
        break;
    default:   
        return ComUtility::OnPublicApiReturn(STG_E_INVALIDFUNCTION);
        break;
    }

    if (::SetFilePointerEx(hFile_, liDistanceToMove, (PLARGE_INTEGER) lpNewFilePointer,
        dwMoveMethod) == 0)
    {
         auto error = GetLastError();
         WriteWarning(
            TraceType_XmlFileStream,
            "SetFilePointerEx failed {0}",
            error);

        return ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(error));
    }
    else
    {
        return ComUtility::OnPublicApiReturn(S_OK);
    }
}

HRESULT STDMETHODCALLTYPE ComXmlFileStream::Stat(STATSTG* pStatstg, DWORD ) 
{
    if (::GetFileSizeEx(hFile_, (PLARGE_INTEGER) &pStatstg->cbSize) == 0)
    {
        auto error = GetLastError();
            WriteWarning(
            TraceType_XmlFileStream,
            "GetFileSizeEx failed {0}",
            error);

        return ComUtility::OnPublicApiReturn(HRESULT_FROM_WIN32(error));
    }
    else
    {
        return ComUtility::OnPublicApiReturn(S_OK);
    }
}
