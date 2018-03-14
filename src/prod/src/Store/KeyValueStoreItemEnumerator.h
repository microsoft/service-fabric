// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class KeyValueStoreItemEnumerator : 
        public Api::IKeyValueStoreItemEnumerator,
        public KeyValueStoreEnumeratorBase
    {
        DENY_COPY(KeyValueStoreItemEnumerator);

    public:
        KeyValueStoreItemEnumerator(
            std::wstring const & keyPrefix,
            bool strictPrefix,
            IStoreBase::EnumerationSPtr const & replicatedStoreEnumeration);
        virtual ~KeyValueStoreItemEnumerator();

        //
        // IKeyValueStoreItemEnumerator methods
        //
        Common::ErrorCode MoveNext() override;
        Common::ErrorCode TryMoveNext(bool & success) override;
        Api::IKeyValueStoreItemResultPtr get_Current() override;

    protected:
        void ResetCurrent();
        
        Common::ErrorCode InitializeCurrent(
            IStoreBase::EnumerationSPtr const &,
            std::wstring const & key,
            FABRIC_SEQUENCE_NUMBER sequenceNumber,
            FILETIME lastModifiedUtc,
            FILETIME lastModifiedOnPrimaryUtc);

    private:
        Common::RwLock lock_;
        Api::IKeyValueStoreItemResultPtr current_;
    };
}
