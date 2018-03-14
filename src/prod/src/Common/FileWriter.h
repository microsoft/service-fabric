// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    class FileWriter : public Common::TextWriter
    {
        DENY_COPY(FileWriter);
    public:

        FileWriter()
            :file_()
        {
        }

        void WriteUnicodeBOM();

        virtual void WriteAsciiBuffer(__in_ecount(ccLen) char const * buf, size_t ccLen);

        virtual void WriteUnicodeBuffer(__in_ecount(ccLen) wchar_t const * buf, size_t ccLen);

        Common::ErrorCode TryOpen(
            std::wstring const& fileName,
            FileShare::Enum share = FileShare::None,
            FileAttributes::Enum attributes = FileAttributes::None);

        virtual void Flush();

        void Close();

        bool IsValid();

    private:
        Common::File file_;
    };
};
