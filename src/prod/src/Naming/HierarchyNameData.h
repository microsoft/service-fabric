// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class HierarchyNameData : public Serialization::FabricSerializable
    {
    public:         
        HierarchyNameData();

        HierarchyNameData(
            HierarchyNameState::Enum nameState,
            UserServiceState::Enum serviceState,
            __int64 subnamesVersion);

        __declspec(property(get=get_NameState, put=set_NameState)) HierarchyNameState::Enum NameState;
        inline HierarchyNameState::Enum get_NameState() const { return nameState_; }
        inline void set_NameState(HierarchyNameState::Enum value) { nameState_ = value; }

        __declspec(property(get=get_IsNameCreated)) bool IsNameCreated;
        inline bool get_IsNameCreated() const { return nameState_ == HierarchyNameState::Created; }

        __declspec(property(get=get_ServiceState, put=set_ServiceState)) UserServiceState::Enum ServiceState;
        inline UserServiceState::Enum get_ServiceState() const { return serviceState_; }
        inline void set_ServiceState(UserServiceState::Enum value) { serviceState_ = value; }

        __declspec(property(get=get_SubnamesVersion)) __int64 SubnamesVersion;
        inline __int64 get_SubnamesVersion() const { return subnamesVersion_; }

        __declspec (property(get=get_IsExplicit, put=set_IsExplicit)) bool IsExplicit;
        inline bool get_IsExplicit() const { return flags_.IsExplicit(); }
        inline void set_IsExplicit(bool value) { flags_.SetExplicitState(value); }

        __declspec (property(get=get_IsForceDelete)) bool const IsForceDelete;
        inline bool const get_IsForceDelete() const { return serviceState_ == Naming::UserServiceState::Enum::ForceDeleting; }

        void IncrementSubnamesVersion() { ++subnamesVersion_; }

        FABRIC_FIELDS_04(nameState_, serviceState_, subnamesVersion_, flags_);

    private:

        struct Flags
        {
        public:
            Flags();
            void WriteTo(__in Common::TextWriter & writer, Common::FormatOptions const &) const;

            __declspec (property(get=get_Value)) int Value;
            int get_Value() const { return value_; }

            bool IsImplicit() const;
            bool IsExplicit() const;
            void SetExplicitState(bool isExplicit);

            FABRIC_PRIMITIVE_FIELDS_01(value_);

        private:
            enum Enum
            {
                None = 0x00, 
                Explicit = 0x01,
                Implicit = 0x02
            };

            int value_;
        };

        HierarchyNameState::Enum nameState_;
        Naming::UserServiceState::Enum serviceState_;
        __int64 subnamesVersion_;
        Flags flags_;
    };
}
