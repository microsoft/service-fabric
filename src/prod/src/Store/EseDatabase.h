// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class EseDatabase : LIST_ENTRY
    {
        DENY_COPY(EseDatabase);
    private:
        EseDatabase(EseSessionSPtr const & session);

    public:
        static std::shared_ptr<EseDatabase> CreateSPtr(EseSessionSPtr const &);
        ~EseDatabase();

        __declspec (property(get=get_DatabaseId)) JET_DBID DatabaseId;

        JET_DBID get_DatabaseId() const { return databaseId_; }

        JET_ERR InitializeCreate(
            std::wstring const & filename);
        JET_ERR InitializeAttachAndOpen(
            std::wstring const & filename);
        JET_ERR InitializeOpen(
            std::wstring const & filename);

    private:
        EseSessionSPtr session_;
        JET_DBID databaseId_;
        std::wstring filename_;
        bool attachedToDatabase_;

        void OnInitializing() const;
    };

    typedef std::shared_ptr<EseDatabase> EseDatabaseSPtr;
}
