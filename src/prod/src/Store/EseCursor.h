// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseCursor
    {
        DENY_COPY(EseCursor);
    public:
        EseCursor(EseDatabaseSPtr const &, EseSessionSPtr const &);
        ~EseCursor();

        __declspec (property(get=get_TableId)) JET_TABLEID TableId;

        JET_TABLEID get_TableId() const { return tableId_; }

        JET_ERR InitializeCreateTable(__in EseTableBuilder & tableBuilder);
        JET_ERR InitializeOpenTable(std::wstring const & tableName);

        // rowCount can be +/- offset, or JET_MoveFirst, or JET_MoveLast.
        JET_ERR Move(LONG rowCount);
        JET_ERR SetCurrentIndex(std::wstring const & indexName);
        JET_ERR MakeKey(std::wstring const & key, JET_GRBIT grbit);
        JET_ERR MakeKey(__int64, JET_GRBIT grbit);
		JET_ERR PrepareForScan();
        JET_ERR Seek(JET_GRBIT grbit);
        
        void MarkTableClosedOnTxRollback() { tableClosedOnTxRollback_ = true; }

        JET_ERR GetColumnInfo(std::wstring const & columnName, __out JET_COLUMNDEF & info) const;
        JET_ERR GetColumnId(std::wstring const & columnName, JET_COLUMNID & columnId) const;
        JET_ERR RetrieveActualSize(JET_COLUMNID columnId, __out size_t & columnSize) const;
        JET_ERR RetrieveColumn(
            JET_COLUMNID columnId,
            __out_bcount_opt(bufferSize) void * buffer,
            size_t bufferSize,
            __out size_t & actualSize) const;

        template <class T>
        JET_ERR RetrieveColumn(JET_COLUMNID columnId, __out T & value) const
        {
            size_t actualSize;
            JET_ERR jetErr = this->RetrieveColumn(columnId, &value, sizeof(value), /*out*/actualSize);
            if (JET_errSuccess == jetErr)
            {
                ASSERT_IF(actualSize != sizeof(value), "variable size value retrieved as fixed size");
            }

            return jetErr;
        }

        template <>
        JET_ERR RetrieveColumn<std::wstring>(JET_COLUMNID columnId, __out std::wstring & value) const
        {
            return this->RetrieveColumnAsString(columnId, /*out*/value);
        }

        template <>
        JET_ERR RetrieveColumn<std::vector<unsigned _int8>>(
            JET_COLUMNID columnId,
            __out std::vector<unsigned _int8> & value) const
        {
            return this->RetrieveColumnAsBinary(columnId, /*out*/value);
        }

        JET_ERR Delete();

        // Options for prep:
        //   JET_prepCancel
        //   JET_prepInsert
        //   JET_prepInsertCopy
        //   JET_prepInsertCopyDeleteOriginal
        //   JET_prepReplace
        //   JET_prepReplaceNoLock
        // See http://msdn.microsoft.com/en-us/library/ms684030(v=EXCHG.10).aspx
        JET_ERR PrepareUpdate(ULONG prep);

        template <class T>
        JET_ERR SetColumn(JET_COLUMNID columnId, T const & data)
        {
            return this->SetColumn(columnId, &data, sizeof(data));
        }

        template <>
        JET_ERR SetColumn<std::wstring>(JET_COLUMNID columnId, std::wstring const & data)
        {
            return this->SetColumn(columnId, data.data(), data.length() * sizeof(wchar_t));
        }

        JET_ERR SetColumn(JET_COLUMNID columnId, __in_opt void const * p, size_t sizeInBytes);
        JET_ERR Update();

        JET_ERR AlterTableAddColumn(
            std::wstring const & columnName,
            JET_COLTYP type,
            size_t maxSize,
            JET_GRBIT grbit,
            __in_bcount_opt(defaultValueSize) void * defaultValue,        // silently truncated to 255 bytes
            __range(0, 255) ULONG defaultValueSize,
            JET_COLUMNID & columnId);

    private:
        JET_ERR MakeKey(void const * buffer, size_t bufferSize, JET_GRBIT grbit);

        JET_ERR RetrieveColumnAsString(
            JET_COLUMNID columnId,
            __out std::wstring & value) const;

        JET_ERR RetrieveColumnAsBinary(
            JET_COLUMNID columnId,
            __out std::vector<unsigned _int8> & value) const;
        
        void OnInitializing() const;

        void AddKey(std::wstring const &, JET_GRBIT);
        std::wstring GetTag(std::wstring const & func) const;

        EseDatabaseSPtr database_;
        EseSessionSPtr session_;
        JET_TABLEID tableId_;
        bool tableClosedOnTxRollback_;
        std::vector<std::wstring> currentKeys_;
    };

    typedef std::unique_ptr<EseCursor> EseCursorUPtr;
    typedef std::shared_ptr<EseCursor> EseCursorSPtr;
}
