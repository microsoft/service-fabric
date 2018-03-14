// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    // Assumptions:
    // Only time insert will happen is when we are accepting the query arguments from the ComNamingClient
    // Hence no locking is used.
    class QueryArgumentMap : public Serialization::FabricSerializable
    {
    public:
        QueryArgumentMap();

        bool ContainsKey(std::wstring const & key) const;

        // Returns false if "key" not found. Input "value" is not touched if "key" not found.
        bool TryGetValue(std::wstring const & key, __out std::wstring & value) const;

        bool TryGetValue(std::wstring const & key, __out int64 & value) const;

        bool TryGetValue(std::wstring const & key, __out Common::Guid & value) const;

        bool TryGetPartitionId(__out Common::Guid & value) const;

        bool TryGetReplicaId(__out int64 & value) const;

        // Returns true is value was found AND could be parsed.
        // Parsed value is stored in 'value's
        bool TryGetBool(std::wstring const & key, __out bool & value) const;

        // Returns success if int64 is found and could be parsed successfully.
        // Returns NameNotFound error is key is not found.
        // Returns InvalidArgument error if value could not be parsed.
        Common::ErrorCode GetInt64(std::wstring const & key, __out int64 & value) const;

        // Returns true is value was found AND could be parsed.
        // Parsed value is stored in 'value'
        bool TryGetInt64(std::wstring const & key, __out int64 & value) const;

        // Returns success if bool is found and could be parsed successfully.
        // Returns NameNotFound error is key is not found.
        // Returns InvalidArgument error if value could not be parsed.
        Common::ErrorCode GetBool(std::wstring const & key, __out bool & value) const;

        // This method does not throw exception but returns empty string if the key is not found.
        // This should be only used after it is known that the key is present in the map.
        std::wstring operator[](std::wstring const &) const ;

        void Insert(std::wstring const & key, std::wstring const & value);

        void Erase(std::wstring const & );

        std::wstring FirstKey();

        static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

        void FillEventData(Common::TraceEventContext &) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        void ToPublicApi(
            __in Common::ScopedHeap & heap,
            __out FABRIC_STRING_PAIR_LIST & publicNodeQueryArgs) const ;

        __declspec(property(get=get_Size)) size_t Size;
        size_t get_Size() const { return queryArgumentMap_.size(); }

        class QueryArgumentMapKey : public Serialization::FabricSerializable
        {
        public:
            QueryArgumentMapKey()
                : key_()
            {
            }

            explicit QueryArgumentMapKey(std::wstring const & key)
                : key_(key)
            {
            }

            bool operator <(const QueryArgumentMap::QueryArgumentMapKey & otherkey) const
            {
                return Common::StringUtility::IsLessCaseInsensitive(key_, otherkey.key_);
            }

            __declspec(property(get=get_Key)) std::wstring const & Key;
            std::wstring const & get_Key() const { return key_; }

            FABRIC_FIELDS_01(key_)
        private:
            std::wstring key_;
        };

        FABRIC_FIELDS_01(queryArgumentMap_)

    private:
        std::map<QueryArgumentMap::QueryArgumentMapKey, std::wstring> queryArgumentMap_;
    };
}

DEFINE_USER_MAP_UTILITY(ServiceModel::QueryArgumentMap::QueryArgumentMapKey, std::wstring)
