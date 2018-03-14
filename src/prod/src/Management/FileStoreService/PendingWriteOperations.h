// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {        
        class PendingWriteOperations
        {
            DENY_COPY(PendingWriteOperations);
        public:

            explicit PendingWriteOperations()
                : closed_(false), set_(), setLock_()
            {
            }

            ~PendingWriteOperations()
            {
            }

            Common::ErrorCode TryAdd(std::wstring const & key, bool const isFolder);

            Common::ErrorCode Contains(std::wstring const & key, __out bool & contains);

            bool Remove(std::wstring const & key);

            void Clear();

            size_t Count();

            // closes the set, so that no more additions are allowed
            std::set<std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> Close();

        private:
            // TODO: A tree implementation would
            // make TryAdd more effecient
            std::set<std::wstring, Common::IsLessCaseInsensitiveComparer<std::wstring>> set_;
            Common::RwLock setLock_;
            bool closed_;
        };        
    }
}
