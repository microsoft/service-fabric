// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Store
{
    // This should be in a Windows header somewhere, but I cannot find it.
    // The ESE documentation explicitly specifies these numbers, and only allows
    // these two possiblities.  :-(
    //
    // See http://msdn.microsoft.com/en-us/library/ms684174(v=EXCHG.10).aspx
#ifdef _UNICODE
    const ULONG defaultCodePage = 1200;
#else // ifdef _UNICODE
    const ULONG defaultCodePage = 1252;
#endif // ifdef _UNICODE

    EseTableBuilder::EseTableBuilder(std::wstring const & tableName)
    {
        memset(&tableDefinition_, 0, sizeof(JET_TABLECREATE));
        tableDefinition_.cbStruct = sizeof(JET_TABLECREATE);
        tableDefinition_.szTableName = SaveString(tableName);
        tableDefinition_.szTemplateTableName = NULL;
        tableDefinition_.ulPages = 1;
        tableDefinition_.ulDensity = 0;
        tableDefinition_.grbit = JET_bitRetrieveHintTableScanForward;
    }

    int EseTableBuilder::AddColumn(
        std::wstring const & columnName,
        JET_COLTYP type,
        size_t maxSize,
        JET_GRBIT grbit,
        __in_bcount_opt(defaultValueSize) void * defaultValue,
        __range(0, 255) ULONG defaultValueSize)
    {
        ASSERT_IF(tableDefinition_.cColumns != 0, "Cannot modify builder after Finalization");
        ASSERT_IF(tableDefinition_.cIndexes != 0, "Cannot modify builder after Finalization");

        JET_COLUMNCREATE init;
        memset(&init, 0, sizeof(init));
        init.cbStruct = sizeof(init);
        init.szColumnName = SaveString(columnName);
        init.coltyp = type;
        init.cbMax = static_cast<ULONG>(maxSize);
        init.grbit = grbit;
        init.pvDefault = defaultValue;
        init.cbDefault = defaultValueSize;
        init.cp = defaultCodePage;
        columnDefinitions_.push_back(init);
        return static_cast<int>(columnDefinitions_.size() - 1);
    }

    int EseTableBuilder::AddIndex(
        std::wstring const & indexName,
        std::wstring const & doubleNullTerminatedKeys,
        JET_GRBIT grbit,
        size_t cbKeyMost)
    {
        // These flags all have to do with indexing multivalued columns, which
        // are not currently supported by EseTableBuilder.  For more information,
        // see http://msdn.microsoft.com/en-us/library/ms684238(EXCHG.10).aspx.
        ASSERT_IF((grbit & JET_bitIndexUnicode) != 0, "Multi-valued columns not supported");
        ASSERT_IF((grbit & JET_bitIndexTuples) != 0, "Multi-valued columns not supported");
        ASSERT_IF((grbit & JET_bitIndexTupleLimits) != 0, "Multi-valued columns not supported");
        ASSERT_IF((grbit & JET_bitIndexCrossProduct) != 0, "Multi-valued columns not supported");

        ASSERT_IF(tableDefinition_.cColumns != 0, "Cannot modify builder after Finalization");
        ASSERT_IF(tableDefinition_.cIndexes != 0, "Cannot modify builder after Finalization");

        int cchKeys = 0;
        for (;;)
        {
            if (doubleNullTerminatedKeys[cchKeys++] == 0)
            {
                if (doubleNullTerminatedKeys[cchKeys++] == 0)
                {
                    break;
                }
            }
        }

        JET_INDEXCREATE init;
        memset(&init, 0, sizeof(init));
        init.cbStruct = sizeof(init);
        init.szIndexName = SaveString(indexName);
        init.szKey = SaveString(doubleNullTerminatedKeys);
        init.cbKey = cchKeys * sizeof(wchar_t);
        init.grbit = grbit;
        init.ulDensity = 80;
        init.lcid = LOCALE_SYSTEM_DEFAULT;
        init.cbVarSegMac = 0;
        init.rgconditionalcolumn = NULL;
        init.cConditionalColumn = 0;
        init.cbKeyMost = static_cast<ULONG>(cbKeyMost);

        indexDefinitions_.push_back(init);
        return static_cast<int>(indexDefinitions_.size() - 1);
    }

    JET_TABLECREATE & EseTableBuilder::FinalizeTableDefinition()
    {
        size_t columnCount = columnDefinitions_.size();
        tableDefinition_.rgcolumncreate = columnDefinitions_.data();
        tableDefinition_.cColumns = static_cast<ULONG>(columnCount);

        size_t indexCount = indexDefinitions_.size();
        tableDefinition_.rgindexcreate = indexDefinitions_.data();
        tableDefinition_.cIndexes = static_cast<ULONG>(indexCount);

        return tableDefinition_;
    }

    JET_COLUMNID EseTableBuilder::GetColumnId(int index)
    {
        ASSERT_IF(index < 0, "negative index");
        ASSERT_IF(index >= static_cast<int>(columnDefinitions_.size()), "index too large");
        const JET_COLUMNCREATE & columnDefinition = columnDefinitions_[index];
        return columnDefinition.columnid;
    }

    JET_ERR EseTableBuilder::GetColumnError(int index)
    {
        ASSERT_IF(index < 0, "negative index");
        ASSERT_IF(index >= static_cast<int>(columnDefinitions_.size()), "index too large");
        const JET_COLUMNCREATE & columnDefinition = columnDefinitions_[index];
        return columnDefinition.err;
    }

    JET_ERR EseTableBuilder::GetIndexError(int index)
    {
        ASSERT_IF(index < 0, "negative index");
        ASSERT_IF(index >= static_cast<int>(columnDefinitions_.size()), "index too large");
        const JET_INDEXCREATE & indexDefinition = indexDefinitions_[index];
        return indexDefinition.err;
    }

    wchar_t * EseTableBuilder::SaveString(
        std::wstring const & s)
    {
        strings_.push_back(s);
        return const_cast<wchar_t *>(strings_.back().c_str());
    }
}
