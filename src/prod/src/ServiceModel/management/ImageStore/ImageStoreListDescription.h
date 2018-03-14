// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageStore
    {
        class ImageStoreListDescription
            : public Common::IFabricJsonSerializable
        {
            DENY_COPY(ImageStoreListDescription);

        public:
            ImageStoreListDescription();
            ImageStoreListDescription(
                std::wstring const & remoteLocation,
                std::wstring const & continuationToken,
                bool isRecursive);

            virtual ~ImageStoreListDescription() {};

            Common::ErrorCode FromPublicApi(FABRIC_IMAGE_STORE_LIST_DESCRIPTION const & listDes);
            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_IMAGE_STORE_LIST_DESCRIPTION &) const;

            __declspec(property(get = get_RemoteLocation, put = set_RemoteLocation)) std::wstring const & RemoteLocation;
            std::wstring const & get_RemoteLocation() const { return remoteLocation_; }
            void set_RemoteLocation(std::wstring const & value) { remoteLocation_ = value; }

            __declspec(property(get = get_ContinuationToken, put = set_ContinuationToken)) std::wstring const & ContinuationToken;
            std::wstring const get_ContinuationToken() const { return continuationToken_; }
            void set_ContinuationToken(std::wstring const & value) { continuationToken_ = value; }

            __declspec(property(get = get_IsRecursive, put = set_IsRecursive)) bool IsRecursive;
            bool get_IsRecursive() const { return isRecursive_; }
            void set_IsRecursive(bool value) { isRecursive_ = value; }

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::RemoteLocation, remoteLocation_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ContinuationToken, continuationToken_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::IsRecursive, isRecursive_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            std::wstring remoteLocation_;
            std::wstring continuationToken_;
            bool isRecursive_;
        };
    }
}
