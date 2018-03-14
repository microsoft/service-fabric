// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {    
        class ImageStoreContentExistsReplyMessageBody : public Serialization::FabricSerializable
        {
            DENY_COPY(ImageStoreContentExistsReplyMessageBody)

        public:
            ImageStoreContentExistsReplyMessageBody()
                : exists_(false)
            {
            }

            ImageStoreContentExistsReplyMessageBody(bool exists)
                : exists_(exists)
            {
            }

            __declspec(property(get = get_Exists)) bool Exists;
            bool get_Exists() const { return this->exists_; }

            FABRIC_FIELDS_01(exists_);

        private:
            bool exists_;
        };
    }
}

