// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

void FileWriter::WriteAsciiBuffer(__in_ecount(ccLen) char const * buf, size_t ccLen)
{
    file_.Write(buf, (int)ccLen);
}

void FileWriter::WriteUnicodeBuffer(__in_ecount(ccLen) wchar_t const * buf, size_t ccLen)  
{ 
    file_.Write(buf, (int)ccLen<<1);
}

Common::ErrorCode FileWriter::TryOpen(
    std::wstring const& fileName,
    FileShare::Enum share,
    FileAttributes::Enum attributes)
{
    return file_.TryOpen(fileName, Common::FileMode::Create, Common::FileAccess::Write, share, attributes);
}

void FileWriter::Flush() 
{
    file_.Flush();
}

void FileWriter::Close()
{
    file_.Close();
}

bool FileWriter::IsValid()
{
    return file_.IsValid();
}

void FileWriter::WriteUnicodeBOM()
{
    byte unicodeBOM[3] = { 0xEF, 0xBB, 0xBF }; 
    file_.Write((const void*)&unicodeBOM, sizeof(unicodeBOM)); 
}
