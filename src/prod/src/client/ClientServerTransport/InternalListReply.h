// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class InternalListReply : public Serialization::FabricSerializable
        {
        public:
            InternalListReply();
            InternalListReply(bool isPresent, FileState::Enum const state, StoreFileVersion const & currentVersion, std::wstring const & storeShareLocation);            

            __declspec(property(get=get_IsPresent)) bool const IsPresent;
            inline bool const get_IsPresent() const { return isPresent_; }

            __declspec(property(get=get_State)) FileState::Enum const State;
            inline FileState::Enum const get_State() const { return state_; }

            __declspec(property(get=get_CurrentVersion)) StoreFileVersion const CurrentVersion;
            inline StoreFileVersion const get_CurrentVersion() const { return currentVersion_; }

            __declspec(property(get=get_StoreShareLocation)) std::wstring const & StoreShareLocation;
            inline std::wstring const & get_StoreShareLocation() const { return storeShareLocation_; }
            
            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            
            FABRIC_FIELDS_04(isPresent_, state_, currentVersion_, storeShareLocation_);            

        private:
            bool isPresent_;
            FileState::Enum state_;
            StoreFileVersion currentVersion_;                                            
            std::wstring storeShareLocation_;
        };
     
    }
}
