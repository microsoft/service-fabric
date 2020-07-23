// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

namespace Store
{
    EseDatabase::EseDatabase(EseSessionSPtr const & session)
        : session_(session), databaseId_(JET_dbidNil), attachedToDatabase_(false)
    {
    }

    EseDatabase::~EseDatabase()
    {
        if ((databaseId_ != JET_dbidNil) || attachedToDatabase_)
        {
            BIND_TO_THREAD( *session_ )

            if (JET_errSuccess == jetErr)
            {
                if (databaseId_ != JET_dbidNil)
                {
                    CALL_ESE_NOTHROW(
                        JetCloseDatabase(
                        session_->SessionId,
                        databaseId_,
                        NULL));
                }

                if (attachedToDatabase_)
                {
                    session_->Instance->DetachDatabase(*session_, filename_);
                }
            }
        }
    }

    EseDatabaseSPtr EseDatabase::CreateSPtr(EseSessionSPtr const & session)
    {
        return shared_ptr<EseDatabase>(new EseDatabase(session));
    }

    JET_ERR EseDatabase::InitializeCreate(
        std::wstring const & filename)
    {
        this->OnInitializing();

        // Copy wstring before we call JET so if we run out of
        // memory we are not partially initialized.
        std::wstring filenameLocal(filename);

        BIND_TO_THREAD( *session_ )

        if (JET_errSuccess == jetErr)
        {
            jetErr = session_->Instance->AttachDatabase(*session_, filenameLocal, false);

            if (JET_errSuccess == jetErr)
            {
                attachedToDatabase_ = true;

                filename_ = std::move(filenameLocal);

                jetErr = CALL_ESE_NOTHROW(
                    JetCreateDatabase(
                    session_->SessionId,
                    filename_.c_str(),
                    NULL,
                    &databaseId_,
                    session_->Instance->GetOptions()));
            }
        }

        return jetErr;
    }

    JET_ERR EseDatabase::InitializeAttachAndOpen(
        std::wstring const & filename)
    {
        this->OnInitializing();

        // Copy wstring before we call JET so if we run out of
        // memory we are not partially initialized.
        std::wstring filenameLocal(filename);

        BIND_TO_THREAD( *session_ )

        if (JET_errSuccess == jetErr)
        {
            jetErr = session_->Instance->AttachDatabase(*session_, filename, true);

            if (JET_errSuccess == jetErr)
            {
                attachedToDatabase_ = true;
                filename_ = std::move(filenameLocal);

                jetErr = CALL_ESE_NOTHROW(
                    JetOpenDatabase(
                    session_->SessionId,
                    filename.c_str(),
                    NULL,
                    &databaseId_,
                    0));
            }
        }

        return jetErr;
    }

    JET_ERR EseDatabase::InitializeOpen(
        std::wstring const & filename)
    {
        this->OnInitializing();

        // Do not set filename_, since we do not want to Detach

        BIND_TO_THREAD( *session_ )

        if (JET_errSuccess == jetErr)
        {
            jetErr = CALL_ESE_NOTHROW(
                JetOpenDatabase(
                session_->SessionId,
                filename.c_str(),
                NULL,
                &databaseId_,
                0));
        }

        return jetErr;
    }

    void EseDatabase::OnInitializing() const
    {
        ASSERT_IF(session_->SessionId == JET_sesidNil, "Session not initialized.");
        ASSERT_IF(databaseId_ != JET_dbidNil, "Cannot initialize twice");
    }
}
