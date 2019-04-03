// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseTableBuilder
    {
        DENY_COPY(EseTableBuilder);
    public:
        EseTableBuilder(std::wstring const & tableName);
        ~EseTableBuilder() { }

        //
        // These parameters are passed to the structure described at:
        //   http://msdn.microsoft.com/en-us/library/ms684174(v=EXCHG.10).aspx
        //
        // Returns a column index that can be used after the table is built to
        // call GetColumnid.
        //
        int AddColumn(
            std::wstring const & columnName,
            JET_COLTYP type,
            size_t maxSize,
            JET_GRBIT grbit,
            __in_bcount_opt(defaultValueSize) void * defaultValue,        // silently truncated to 255 bytes
            __range(0, 255) ULONG defaultValueSize);

        template <int JETCOLTYP> int AddColumn(std::wstring const & columnName, JET_GRBIT grbit)
        {
            static_assert(false, "No AddColumn specialization defined for this type.");
        }

        template <int JETCOLTYP> int AddColumn(std::wstring const & columnName, size_t size, JET_GRBIT grbit)
        {
            static_assert(false, "No AddColumn specialization defined for this type.");
        }

        template <> int AddColumn<JET_coltypBit>(std::wstring const & columnName, JET_GRBIT grbit)
        {
            return this->AddColumn(columnName, JET_coltypBit, sizeof(unsigned _int8), grbit, nullptr, 0);
        }

        template <> int AddColumn<JET_coltypUnsignedByte>(std::wstring const & columnName, JET_GRBIT grbit)
        {
            return this->AddColumn(columnName, JET_coltypUnsignedByte, sizeof(unsigned _int8), grbit, nullptr, 0);
        }

        template <> int AddColumn<JET_coltypLong>(std::wstring const & columnName, JET_GRBIT grbit)
        {
            return this->AddColumn(columnName, JET_coltypLong, sizeof(_int32), grbit, nullptr, 0);
        }

        template <> int AddColumn<JET_coltypLongLong>(std::wstring const & columnName, JET_GRBIT grbit)
        {
            return this->AddColumn(columnName, JET_coltypLongLong, sizeof(_int64), grbit, nullptr, 0);
        }

        template <> int AddColumn<JET_coltypLongText>(std::wstring const & columnName, size_t size, JET_GRBIT grbit)
        {
            return this->AddColumn(columnName, JET_coltypLongText, size, grbit, nullptr, 0);
        }

        template <> int AddColumn<JET_coltypLongBinary>(std::wstring const & columnName, size_t size, JET_GRBIT grbit)
        {
            return this->AddColumn(columnName, JET_coltypLongBinary, size, grbit, NULL, 0);
        }

        //
        // These parameters are passed to the structure described at:
        //   http://msdn.microsoft.com/en-us/library/ms684238(v=EXCHG.10).aspx
        //
        // Returns an index index that can be used after the table is built to
        // call GetIndexId.
        //
        int AddIndex(
            std::wstring const & indexName,
            std::wstring const & doubleNullTerminatedKeys,
            JET_GRBIT grbit,
            size_t cbMaxKey);

        JET_TABLECREATE & FinalizeTableDefinition();

        // Only valid after the table has been built.
        JET_COLUMNID GetColumnId(int index);
        JET_ERR GetColumnError(int index);
        JET_ERR GetIndexError(int index);

    private:
        std::list<std::wstring> strings_;
        std::vector<unsigned _int8> bytes_;
        std::vector<JET_COLUMNCREATE> columnDefinitions_;
        std::vector<JET_INDEXCREATE> indexDefinitions_;
        JET_TABLECREATE tableDefinition_;

        // This returns a stable raw pointer whose lifetime is the same as the EseTableBuilder.
        // The pointer is appropriate for use in ESE data structures that require pointers to
        // strings.
        wchar_t * SaveString(
            std::wstring const & s);
    };
}
