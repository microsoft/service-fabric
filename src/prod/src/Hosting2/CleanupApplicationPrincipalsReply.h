// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CleanupApplicationPrincipalsReply : public Serialization::FabricSerializable
    {
    public:
        CleanupApplicationPrincipalsReply();
        CleanupApplicationPrincipalsReply(
            Common::ErrorCode const & error,
            std::vector<std::wstring> const & deletedAppIds);
            
        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get = get_DeletedAppIds)) std::vector<std::wstring> const & DeletedAppIds;
        std::vector<std::wstring> const & get_DeletedAppIds() const { return deletedAppIds_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(error_, deletedAppIds_);

    private:
        Common::ErrorCode error_;
        std::vector<std::wstring> deletedAppIds_;
    };
}
