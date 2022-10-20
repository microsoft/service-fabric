#include "stdafx.h"

#ifdef PLATFORM_UNIX

#include <arpa/inet.h>

typedef void* LPWSAPROTOCOL_INFOW;

static INT WSAAPI WSAStringToAddressW(
    LPWSTR	AddressString,
    INT		AddressFamily,
    LPWSAPROTOCOL_INFOW lpProtocolInfo,
    LPSOCKADDR 	lpAddress,
    LPINT	lpAddressLength
)
{
    memset(lpAddress, *lpAddressLength, 0);
    std::string addrStr;
    Common::StringUtility::Utf16ToUtf8(AddressString, addrStr);

    void * ipPtr = nullptr;
    int outputSize = *lpAddressLength;
    if (AddressFamily == AF_INET)
    {
        if (*lpAddressLength < sizeof(sockaddr_in))
        {
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        outputSize = sizeof(sockaddr_in);
        ipPtr = &(((sockaddr_in*)lpAddress)->sin_addr);
    }
    else if (AddressFamily == AF_INET6)
    {
        if (*lpAddressLength < sizeof(sockaddr_in6))
        {
            SetLastError(WSAEFAULT);
            return SOCKET_ERROR;
        }

        outputSize = sizeof(sockaddr_in6);
        ipPtr = &(((sockaddr_in6*)lpAddress)->sin6_addr);
    }
    else
    {
        SetLastError(WSAEFAULT);
        return SOCKET_ERROR;
    }

    memset(lpAddress, *lpAddressLength, 0);
    if (inet_pton(AddressFamily, addrStr.c_str(), ipPtr) == 1)
    {
        lpAddress->sa_family = AddressFamily;
        *lpAddressLength = outputSize;
        return 0;
    }

    SetLastError(WSAEINVAL);
    return SOCKET_ERROR;
}

#endif

using namespace std;
using namespace Common;
using namespace Management::NetworkInventoryManager;

StringLiteral const NIMNetworkAllocationPoolProvider("NIMNetworkAllocationPool");

template <class T>
NIMNetworkAllocationPool<T>::NIMNetworkAllocationPool()
{}

template <class T>
NIMNetworkAllocationPool<T>::NIMNetworkAllocationPool(T low, T high) :
    poolBottom_(low),
    poolTop_(high),
    currentBottom_(low)
{
    this->UpdateFreeCount();
}

template <class T>
ErrorCode NIMNetworkAllocationPool<T>::InitializeScalar(T low, T high)
{
    this->poolBottom_ = low;
    this->poolTop_ = high;
    this->currentBottom_ = low;

    this->UpdateFreeCount();

    return ErrorCode(ErrorCodeValue::Success);
}

template <class T>
void NIMNetworkAllocationPool<T>::UpdateFreeCount()
{
    freeCount_ = static_cast<uint>(this->poolTop_ - this->poolBottom_ + this->freeElements_.size());
}

template <class T>
T NIMNetworkAllocationPool<T>::TakeNextElement()
{
    T element = 0;

    // DARIO BUGBUG: for now ignore the forbidden blocks.
    element = this->currentBottom_;
    this->currentBottom_++;
    this->freeCount_--;

    return element;
}

template <class T>
ErrorCode NIMNetworkAllocationPool<T>::ReserveElements(uint count, std::vector<T>& elements)
{
    ErrorCode error(ErrorCodeValue::Success);

    //--------
    // Make sure I have enough and won't run of elements inside.
    if (freeCount_ > count)
    {
        AcquireExclusiveLock lock(this->lock_);

        while (count-- > 0)
        {
            if (this->freeElements_.size())
            {
                elements.push_back(this->freeElements_.back());
                this->freeElements_.pop_back();
                this->freeCount_--;
            }
            else
            {
                //--------
                // No elements in the free list, grab new ones.
                elements.push_back(this->TakeNextElement());
            }
        }
    }
    else
    {
        WriteError(NIMNetworkAllocationPoolProvider, "ReserveElements: no more free elements in the pool: [{0}]",
            error.ErrorCodeValueToString());

        error = ErrorCodeValue::OutOfMemory;
    }

    return error;
}

template <class T>
ErrorCode NIMNetworkAllocationPool<T>::ReturnElements(std::vector<T>& elements)
{
    ErrorCode error(ErrorCodeValue::Success);

    {
        AcquireExclusiveLock lock(this->lock_);
        for (auto i : elements)
        {
            this->freeElements_.push_front(i);
        }

        this->freeCount_ += static_cast<uint>(elements.size());
    }

    return error;
}

//--------
// Explicit template instantiation for the sizes we support.
template class Management::NetworkInventoryManager::NIMNetworkAllocationPool<uint>;         // ipv4 address pools.
template class Management::NetworkInventoryManager::NIMNetworkAllocationPool<uint64>;       // MAC address pools.


ErrorCode NIMIPv4AllocationPool::Initialize(const std::wstring ipAddressBottom,
    const std::wstring ipAddressTop)
{
    ErrorCode error(ErrorCodeValue::Success);

    uint bottom = 0;
    uint top = 0;

    error = this->GetScalarFromIpAddress(ipAddressBottom, bottom);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = this->GetScalarFromIpAddress(ipAddressTop, top);
    if (!error.IsSuccess())
    {
        return error;
    }

    error = this->InitializeScalar(bottom, top);

    return error;
}


//--------
// Initialize the IP pool based on subnet, prefix and addresses to skip.
ErrorCode NIMIPv4AllocationPool::Initialize(const std::wstring ipSubnet,
    int prefix,
    int addressesToSkip)
{
    ErrorCode error(ErrorCodeValue::Success);
    uint bottom = 0;

    if (prefix > 30)
    {
        WriteError(NIMNetworkAllocationPoolProvider, "Initialize: invalid subnet prefix: [{0}]",
            prefix);

        error = ErrorCode(ErrorCodeValue::InvalidArgument);
        return error;
    }

    if (!(error = this->GetScalarFromIpAddress(ipSubnet, bottom)).IsSuccess())
    {
        return error;
    }

    uint top = bottom + (1 << (32 - prefix)) - 1;

    //--------
    // Include network and gw address at least.
    if (addressesToSkip == 0)
    {
        addressesToSkip = 2;
    }
    bottom += addressesToSkip;

    error = this->InitializeScalar(bottom, top);

    return error;
}


ErrorCode NIMIPv4AllocationPool::GetScalarFromIpAddress(const std::wstring ipStr, uint & value)
{
    ErrorCode error(ErrorCodeValue::Success);

    ::sockaddr address;
    int size = sizeof(address);
    ::ZeroMemory(&address, size);

    int wsaError = ::WSAStringToAddressW(const_cast<LPWSTR>(ipStr.c_str()), AF_INET, nullptr, &address, &size);

    if (0 == wsaError)
    {
        struct sockaddr_in *sin = (struct sockaddr_in *) &address;
#if defined(PLATFORM_UNIX)
        value = htonl((sin->sin_addr).s_addr);
#else
        value = htonl((sin->sin_addr).S_un.S_addr);
#endif
    }
    else
    {
        SetLastError(WSAGetLastError());
        error = ErrorCode::FromWin32Error();

        WriteError(NIMNetworkAllocationPoolProvider, "GetScalarFromIpAddress: could not convert IP string: [{0}], error: {1}",
            ipStr, error);
    }

    return error;
}

std::wstring NIMIPv4AllocationPool::GetIpString(uint scalar)
{
    std::wstring ipString;
    ::sockaddr address;
    int size = sizeof(address);
    ::ZeroMemory(&address, size);

    struct sockaddr_in *sin = (struct sockaddr_in *) &address;
#if defined(PLATFORM_UNIX)
    (sin->sin_addr).s_addr = htonl(scalar);
#else
    (sin->sin_addr).S_un.S_addr = htonl(scalar);
#endif

#ifdef PLATFORM_UNIX
    char buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sin->sin_addr, (PSTR)buffer, sizeof(buffer));

    StringUtility::Utf8ToUtf16(string(buffer), ipString);
#else 
    WCHAR buffer[INET_ADDRSTRLEN];
    InetNtop(AF_INET, &sin->sin_addr, buffer, sizeof(buffer));

    ipString = std::wstring(buffer);
#endif

    return ipString;
}


ErrorCode NIMMACAllocationPool::Initialize(const std::wstring macAddressBottom, const std::wstring macAddressTop)
{
    ErrorCode error(ErrorCodeValue::Success);

    uint64 bottom = 0;
    uint64 top = 0;

    error = this->GetScalarFromMacAddress(macAddressBottom, bottom);
    if (error.IsSuccess())
    {
        error = this->GetScalarFromMacAddress(macAddressTop, top);

        if (error.IsSuccess())
        {
            error = this->InitializeScalar(bottom, top);
        }
    }

    return error;
}

std::wstring NIMMACAllocationPool::GetBottomMAC() const
{
    return GetMacString(this->poolBottom_);
}

std::wstring NIMMACAllocationPool::GetTopMAC() const
{
    return GetMacString(this->poolTop_);
}

ErrorCode NIMMACAllocationPool::GetScalarFromMacAddress(const std::wstring macStr, uint64 & value)
{
    ErrorCode error(ErrorCodeValue::Success);

    std::vector<std::wstring> tokens;
    StringUtility::Split<std::wstring>(macStr, tokens, L"-");

    if (tokens.size() == 6)
    {
        auto i = std::vector<std::wstring>::const_iterator();
        for (i = tokens.begin(); i < tokens.end(); i++)
        {
            uint64 part = 0;
            if (!StringUtility::ParseIntegralString<uint64, false>::Try(*i, part, 16U))
            {
                WriteError(NIMNetworkAllocationPoolProvider, "GetScalarFromMacAddress: could not convert string to integer: [{0}]",
                    *i);

                error = ErrorCode(ErrorCodeValue::InvalidAddress);
                break;
            }

            value = (value << 8) + part;
        }
    }
    else
    {
        WriteError(NIMNetworkAllocationPoolProvider, "GetScalarFromMacAddress: string is not a MAC address: [{0}]",
            macStr);

        error = ErrorCode(ErrorCodeValue::InvalidAddress);
    }

    return error;
}

static const FormatOptions hexByteFormat(2, true, "x");

std::wstring NIMMACAllocationPool::GetMacString(uint64 scalar) const
{
    wstring macStr;
    StringWriter writer(macStr);

    for (int i = 40; i >= 0; i -= 8)
    {
        uint64 byteToWrite = (scalar >> i) & 0xff;
        writer.WriteNumber(byteToWrite, hexByteFormat, false);
        if (i > 0) writer.WriteChar(L'-');
    }

    return macStr;
}
