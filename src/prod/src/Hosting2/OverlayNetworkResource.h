// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    // This class supports the following features -
    // 1) An instance of this class represents a single network resource in a pool,
    // including its current reservation state.
    // 2) Built in comparator and hasher functions, to allow the set container to figure out if the network resource is unique.
    class OverlayNetworkResource
    {
        DENY_COPY(OverlayNetworkResource);

    public:
        OverlayNetworkResource(
            uint ipAddress,
            uint64 macAddress);

        OverlayNetworkResource(
            uint ipAddress,
            uint64 macAddress,
            wstring reservationId);

        OverlayNetworkResource(
            wstring ipAddress,
            wstring macAddress);

        virtual ~OverlayNetworkResource() {};

        __declspec(property(get = get_IPV4Address)) uint IPV4Address;
        uint get_IPV4Address() const { return this->IPV4Address_; }

        __declspec(property(get = get_MacAddress)) uint64 MacAddress;
        uint64 get_MacAddress() const { return this->macAddress_; }

        __declspec(property(get = get_ReservationId, put = set_ReservationId)) const std::wstring &ReservationId;
        const std::wstring &get_ReservationId() const { return this->reservationId_; }
        void set_ReservationId(const std::wstring &reservation) { this->reservationId_ = reservation; }

        // OverlayNetworkResourceSetHasher: Function object that provides hash function
        struct OverlayNetworkResourceSetHasher
        {
            std::size_t operator()(const OverlayNetworkResourceSPtr & k) const
            {
                return StringUtility::GetHash(wformatString("{0}:{1}", k->get_IPV4Address(), k->get_MacAddress()));
            }
        };

        // OverlayNetworkResourceSetComparator: Function object that provides comparison function
        struct OverlayNetworkResourceSetComparator
        {
            bool operator() (const OverlayNetworkResourceSPtr & left, const OverlayNetworkResourceSPtr & right) const
            {
                // need unique pair of ip and mac address
                if ((left->get_IPV4Address() == right->get_IPV4Address()) && (left->get_MacAddress() == right->get_MacAddress()))
                {
                    return true;
                }

                return false;
            }
        };

        // FormatAsString: Converts the network resource into a readable string
        std::wstring FormatAsString();

    private:
        uint IPV4Address_;
        uint64 macAddress_;
        std::wstring reservationId_;
    };
}