// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace DNS
{
    template <class T>
    class ComPointer
    {
    public:
        ComPointer()
            : _pRawPtr(nullptr)
        {
        }

        ComPointer(
            __in_opt T* other
            ) : _pRawPtr(other)
        {
            if (_pRawPtr != nullptr)
            {
                _pRawPtr->AddRef();
            }
        }

        ComPointer(
            __in const ComPointer& other
            ) : _pRawPtr(other._pRawPtr)
        {
            if (_pRawPtr != nullptr)
            {
                _pRawPtr->AddRef();
            }
        }

        ~ComPointer()
        {
            Release();
        }

        ComPointer& operator = (
            __in_opt T* other
            ) 
        {
            if (_pRawPtr != other)
            {
                Release();

                _pRawPtr = other;

                if (_pRawPtr != nullptr)
                {
                    _pRawPtr->AddRef();
                }
            }
            return *this;
        }

        ComPointer& operator = (
            __in const ComPointer& other
            ) 
        {
            if (_pRawPtr != other._pRawPtr)
            {
                Release();

                _pRawPtr = other._pRawPtr;

                if (_pRawPtr != nullptr)
                {
                    _pRawPtr->AddRef();
                }
            }
            return *this;
        }

        bool operator == (
            __in_opt T* other
            ) const
        {
            return (_pRawPtr == other);
        }

        bool operator != (
            __in_opt T* other
            ) const
        {
            return (_pRawPtr != other);
        }

        bool operator == (
            __in const ComPointer& other
            ) const
        {
            return (_pRawPtr == other.RawPtr());
        }

        bool operator != (
            __in const ComPointer& other
            ) const
        {
            return (_pRawPtr != other.RawPtr());
        }

        T* operator -> () const
        {
            return _pRawPtr;
        }

        void Release() 
        {
            if (_pRawPtr != nullptr)
            {
                _pRawPtr->Release();
                _pRawPtr = nullptr;
            }
        }

        void Attach(
            __in_opt T* other
            )
        {
            Release();
            _pRawPtr = other;
        }

        T* Detach()
        {
            T* pResult = _pRawPtr;
            _pRawPtr = nullptr;

            return pResult;
        }

        T* RawPtr() const 
        {
            return _pRawPtr;
        }

        T** InitializationAddress()
        {
            return &_pRawPtr;
        }

        void** VoidInitializationAddress()
        {
            return reinterpret_cast<void**>(&_pRawPtr);
        }

    private:
        T* _pRawPtr;
    };
}
