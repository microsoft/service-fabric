// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

using namespace Common;

DEFINE_AS_BLITTABLE(uint);
DEFINE_AS_BLITTABLE(uint64);

namespace Management
{
    namespace NetworkInventoryManager
    {
        //--------
        // This class encapsulates a generic numeric pool that can potentially be very big.
        // It allocates elements from the given, normalized, range. 
        // As elements are freed they end up in a free element list and next allocation is fulfilled from there.
        // It is later on specialized for IP and MAC address pools.
        // TODO: Make it generic for ipv6 in the future?
        template <class T>
        class NIMNetworkAllocationPool : public Common::TextTraceComponent<Common::TraceTaskCodes::FM>, public Serialization::FabricSerializable
        {
        public:
            NIMNetworkAllocationPool();
            NIMNetworkAllocationPool(T low, T high);

            ErrorCode InitializeScalar(T low, T high);

            ErrorCode ReserveElements(uint count,
                __out std::vector<T>& elements);

            ErrorCode ReturnElements(std::vector<T>& elements);

            __declspec(property(get = get_FreeCount)) int FreeCount;
            int get_FreeCount() { return freeCount_; }

            FABRIC_FIELDS_05(poolBottom_, poolTop_, currentBottom_, freeCount_, freeElements_); // DARIO_BUGBUG add freeElements!

        protected:
            //--------
            // Update the count of free elements.
            void UpdateFreeCount();

            //--------
            // Get the next element from the pool, updating the inner state.
            T TakeNextElement();

            T poolBottom_;
            T poolTop_;
            T currentBottom_;
            uint freeCount_;

            //std::vector<std::pair<T, T>> forbidenRanges_;
            std::deque<T> freeElements_;

            ExclusiveLock lock_;
        };

        //--------
        // Address pool specialization for IPv4 addresss.
        class NIMIPv4AllocationPool : public NIMNetworkAllocationPool<uint>
        {
        public:
            NIMIPv4AllocationPool()
            {}

            //--------
            // Initialize the pool using an IPv4 range.
            ErrorCode Initialize(const std::wstring ipAddressBottom, const std::wstring ipAddressTop);

            //--------
            // Initialize the pool using subnet information and addresses to skip from the beginning.
            ErrorCode Initialize(const std::wstring ipSubnet, int prefix, int addressesToSkip);

            //--------
            // Get the string representation for the IPv4 address in dot format.
            static std::wstring GetIpString(uint scalar);

            //--------
            // Get the scalar representation for the IPv4 string (in dot format)
            static ErrorCode GetScalarFromIpAddress(const std::wstring ipStr, uint & value);

        private:

        };

        //--------
        // Address pool specialization for MAC addresss.
        class NIMMACAllocationPool : public NIMNetworkAllocationPool<uint64>
        {
        public:
            NIMMACAllocationPool() {}

            //--------
            // Initializes the MAC pool with a range of MACs
            ErrorCode Initialize(const std::wstring macAddressBottom, const std::wstring macAddressTop);

            //--------
            // Get the bottom MAC address.
            std::wstring GetBottomMAC() const;

            //--------
            // Get the top MAC address.
            std::wstring GetTopMAC() const;

            //--------
            // Get the string representation for the MAC in 00-00-00-00-00-01 format.
            std::wstring GetMacString(uint64 scalar) const;

            //--------
            // Get the scalar representation for the MAC string (in 00-00-00-00-00-01 format)
            ErrorCode GetScalarFromMacAddress(const std::wstring ipStr, uint64 & value);

        private:

        };

        //--------
        // Address pool specialization for MAC addresss.
        class NIMVSIDAllocationPool : public NIMNetworkAllocationPool<uint>
        {
        public:
            NIMVSIDAllocationPool() {}
        };

        typedef std::shared_ptr<NIMMACAllocationPool> MACAllocationPoolSPtr;
        typedef std::shared_ptr<NIMIPv4AllocationPool> IPv4AllocationPoolSPtr;
        typedef std::shared_ptr<NIMVSIDAllocationPool> VSIDAllocationPoolSPtr;
    }
}