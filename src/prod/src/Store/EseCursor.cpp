// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

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

#define BIND_TO_THREAD_WTAG BIND_TO_THREAD_TAG( *session_, this->GetTag( TO_WSTRING( __FUNCTION__ ) ) )

    EseCursor::EseCursor(EseDatabaseSPtr const & database, EseSessionSPtr const & session)
        : database_(database)
        , session_(session)
        , tableId_(JET_tableidNil)
        , tableClosedOnTxRollback_(false)
        , currentKeys_()
    {
    }

    EseCursor::~EseCursor()
    {
        if (!tableClosedOnTxRollback_ && tableId_ != JET_tableidNil)
        {
            BIND_TO_THREAD_WTAG

            if (JET_errSuccess == jetErr)
            {
                CALL_ESE_NOTHROW(JetCloseTable(session_->SessionId, tableId_));
            }
        }
    }
	
	JET_ERR EseCursor::AlterTableAddColumn(
        std::wstring const & columnName,
        JET_COLTYP colType,
        size_t maxSize,
        JET_GRBIT grbit,
        __in_bcount_opt(defaultValueSize) void * defaultValue,
        __range(0, 255) ULONG defaultValueSize,
        JET_COLUMNID & columnId)
    {
        JET_COLUMNDEF colDef;
        memset(&colDef, 0, sizeof(colDef));        
        colDef.cbStruct = sizeof(colDef);
        colDef.columnid = 0;
        colDef.coltyp = colType;
        colDef.wCountry = 0;
        colDef.langid = 0;
        colDef.cp = defaultCodePage;
        colDef.wCollate = 0;
        colDef.cbMax = static_cast<ULONG>(maxSize);
        colDef.grbit = grbit;

        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetAddColumn(
                    session_->SessionId,
                    tableId_,
                    columnName.c_str(),
                    &colDef,
                    defaultValue,
                    defaultValueSize,
                    &columnId));
        }

        return jetErr;
    }

    JET_ERR EseCursor::InitializeCreateTable(
        __in EseTableBuilder & tableBuilder)
    {
        this->OnInitializing();
        auto tableDefinition = tableBuilder.FinalizeTableDefinition();

        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetCreateTableColumnIndex(
                session_->SessionId,
                database_->DatabaseId,
                &tableDefinition));
        }

        if (JET_errSuccess == jetErr)
        {
            tableId_ = tableDefinition.tableid;
        }

        return jetErr;
    }

    JET_ERR EseCursor::InitializeOpenTable(
        std::wstring const & tableName)
    {
        this->OnInitializing();

        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            auto flags = JET_bitTableUpdatable;

            if (session_->Instance->Settings.OptimizeTableScans)
            {
                flags |= (JET_bitTablePreread | JET_bitTableSequential);
            }

            jetErr = CALL_ESE_NOTHROW(
                JetOpenTable(
                session_->SessionId,
                database_->DatabaseId,
                tableName.c_str(),
                NULL,
                0,
                flags,
                &tableId_));
        }

        return jetErr;
    }

    void EseCursor::OnInitializing() const
    {
        ASSERT_IF(tableId_ != JET_tableidNil, "Cannot initialize twice");
        ASSERT_IF(database_->DatabaseId == JET_dbidNil, "Database has not yet been initialized");
    }

    JET_ERR EseCursor::Move(
        LONG offset)
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_CHECK_NOTHROW(
                &EseException::ExpectSuccessOrNoRecord,
                JetMove(session_->SessionId, tableId_, offset, 0));
        }

        return jetErr;
    }

    JET_ERR EseCursor::SetCurrentIndex(
        std::wstring const & indexName)
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(JetSetCurrentIndex(session_->SessionId, tableId_, indexName.c_str()));
        }

        return jetErr;
    }

    JET_ERR EseCursor::MakeKey(
        wstring const & key,
        JET_GRBIT grbit)
    {
        AddKey(key, grbit);

        return this->MakeKey(
            key.data(), 
            static_cast<ULONG>(key.size() * sizeof(wchar_t)), 
            grbit);
    }

    JET_ERR EseCursor::MakeKey(
        __int64 key,
        JET_GRBIT grbit)
    {
        AddKey(wformatString("int64({0})", key), grbit);

        return this->MakeKey(&key, sizeof(key), grbit);
    }

    JET_ERR EseCursor::MakeKey(
        void const * buffer,
        size_t bufferSize,
        JET_GRBIT grbit)
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetMakeKey(
                session_->SessionId,
                tableId_,
                buffer,
                static_cast<ULONG>(bufferSize),
                grbit));
        }

        return jetErr;
    }

    JET_ERR EseCursor::PrepareForScan()
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_CHECK_NOTHROW(
                &EseException::ExpectSuccess,
                JetSetTableSequential(session_->SessionId, tableId_, JET_bitPrereadForward));
        }

        return jetErr;
    }

    JET_ERR EseCursor::Seek(
        JET_GRBIT grbit)
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_CHECK_NOTHROW(
                &EseException::ExpectSuccessOrNotFound,
                JetSeek(session_->SessionId, tableId_, grbit));
        }

        return jetErr;
    }

    JET_ERR EseCursor::GetColumnInfo(
        std::wstring const & columnName,
        __out JET_COLUMNDEF & info) const
    {
        memset(&info, 0, sizeof(info));
        info.cbStruct = sizeof(info);

        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetGetTableColumnInfo(
                session_->SessionId,
                tableId_,
                columnName.c_str(),
                &info,
                sizeof(info),
                JET_ColInfo));
        }

        return jetErr;
    }

    JET_ERR EseCursor::GetColumnId(
        std::wstring const & columnName,
        JET_COLUMNID & columnId ) const
    {
        JET_COLUMNDEF info;

        JET_ERR jetErr = this->GetColumnInfo(columnName, /*out*/info);
        if (JET_errSuccess == jetErr)
        {
            columnId = info.columnid;
        }

        return jetErr;
    }

    JET_ERR EseCursor::RetrieveActualSize(
        JET_COLUMNID columnId,
        size_t & columnSize) const
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            ULONG actualSize32;

            jetErr = CALL_ESE_CHECK_NOTHROW(
                &EseException::ExpectSuccessOrTruncate,
                JetRetrieveColumn(
                session_->SessionId,
                tableId_,
                columnId,
                NULL,
                0,
                &actualSize32,
                0,
                NULL));

            if (JET_wrnBufferTruncated == jetErr)
            {
                jetErr = JET_errSuccess;
            }

            if (JET_errSuccess == jetErr)
            {
                columnSize = static_cast<size_t>(actualSize32);
            }
        }

        return jetErr;
    }

    JET_ERR EseCursor::RetrieveColumn(
        JET_COLUMNID columnId,
        __out_bcount_opt(bufferSize) void * buffer,
        size_t bufferSize,
        __out size_t & actualSize) const
    {
        ULONG actualSize32;

        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetRetrieveColumn(
                session_->SessionId,
                tableId_,
                columnId,
                buffer,
                static_cast<ULONG>(bufferSize),
                &actualSize32,
                0,
                NULL));

            if (JET_errSuccess == jetErr)
            {
                actualSize = static_cast<size_t>(actualSize32);
            }
        }

        return jetErr;
    }

    JET_ERR EseCursor::RetrieveColumnAsString(
        JET_COLUMNID columnId,
        __out std::wstring & value) const
    {
        value.clear();
        size_t size;

        JET_ERR jetErr = this->RetrieveActualSize(columnId, size);

        if (JET_errSuccess == jetErr)
        {
            value.resize(size / sizeof(wchar_t));
            auto p = const_cast<void *>(reinterpret_cast<void const *>(value.data()));

            size_t actualSize;
            jetErr = this->RetrieveColumn(columnId, p, size, /*out*/actualSize);
            ASSERT_IF(
                JET_errSuccess == jetErr && actualSize != size,
                "binary value changed size while retrieving - actualSize {0} != size {1}.", actualSize, size);
        }

        return jetErr;
    }

    JET_ERR EseCursor::RetrieveColumnAsBinary(
        JET_COLUMNID columnId,
        __out std::vector<unsigned _int8> & value) const
    {
        value.clear();
        size_t size;
        JET_ERR jetErr = this->RetrieveActualSize(columnId, size);

        if (JET_errSuccess == jetErr)
        {
            value.resize(size);
            auto p = const_cast<void *>(reinterpret_cast<void const *>(value.data()));

            size_t actualSize;
            jetErr = this->RetrieveColumn(columnId, p, size, /*out*/actualSize);
            ASSERT_IF(
                JET_errSuccess == jetErr && actualSize != size,
                "string value changed size while retrieving - actualSize {0} != size {1}.", actualSize, size);
        }

        return jetErr;
    }

    JET_ERR EseCursor::Delete()
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetDelete(
                session_->SessionId,
                tableId_));
        }

        return jetErr;
    }

    JET_ERR EseCursor::PrepareUpdate(
        ULONG prep)
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetPrepareUpdate(
                session_->SessionId,
                tableId_,
                prep));
        }

        return jetErr;
    }

    JET_ERR EseCursor::SetColumn(
        JET_COLUMNID columnId,
        __in_opt void const * p,
        size_t sizeInBytes)
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            JET_GRBIT flags = ((p == NULL) ? JET_bitSetZeroLength : 0);

            auto threshold = session_->Instance->Settings.IntrinsicValueThresholdInBytes;
            if (threshold > 0 && sizeInBytes <= static_cast<size_t>(threshold))
            {
                flags |= JET_bitSetIntrinsicLV;
            }

            if (session_->Instance->Settings.EnableOverwriteOnUpdate)
            {
                flags |= JET_bitSetOverwriteLV;
            }

            jetErr = CALL_ESE_NOTHROW(JetSetColumn(
                session_->SessionId,
                tableId_,
                columnId,
                p,
                static_cast<ULONG>(sizeInBytes),
                flags,
                NULL));
        }

        return jetErr;
    }

    JET_ERR EseCursor::Update()
    {
        BIND_TO_THREAD_WTAG

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetUpdate(
                session_->SessionId,
                tableId_,
                NULL,
                0,
                NULL));
        }
        return jetErr;
    }

    void EseCursor::AddKey(std::wstring const & key, JET_GRBIT grbit)
    {
        if (grbit == JET_bitNewKey)
        {
            currentKeys_.clear();
        }

        currentKeys_.push_back(key);
    }

    wstring EseCursor::GetTag(wstring const & func) const
    {
        wstring tag(func);
        tag.append(L"(");
        for (auto ix=0; ix<currentKeys_.size(); ++ix)
        {
            if (ix > 0)
            {
                tag.append(L", ");
            }

            tag.append(currentKeys_[ix]);
        }
        tag.append(L")");

        return tag;
    }
}
