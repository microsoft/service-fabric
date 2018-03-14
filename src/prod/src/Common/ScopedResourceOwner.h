// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common 
{

    template<typename T>
    class AcquireTraits 
    {
    protected:
        static bool TryAcquire(T *v) 
        {
            return v->TryAcquire();
        }

        template<typename P>
        static bool TryAcquire(T *v, P param) 
        {
            return v->TryAcquire(param);
        }
        
        static void Acquire(T *v) 
        {
            v->Acquire();
        }
        
        static void Release(T *v) 
        {
            v->Release();
        }
    };
    
    template<typename T>
    class AcquireSharedTraits 
    {
    protected:
        _Acquires_shared_lock_(v)
        static void Acquire(T *v) 
        {
            v->AcquireShared();
        }

        _Releases_shared_lock_(v)
        static void Release(T *v) 
        {
            v->ReleaseShared();
        }
    };
    
    template<typename T>
    class AcquireExclusiveTraits 
    {
    protected:
        _Acquires_exclusive_lock_(v)
        static void Acquire(T *v) 
        {
            v->AcquireExclusive();
        }

        _Releases_exclusive_lock_(v)
        static void Release(T *v) 
        {
            v->ReleaseExclusive();
        }
    };

    // A template class that uses RAII to ensure that an object will be propertly
    // unlocked. Class is inherited from ScopedObj which allows avoide
    // specifying template parameters in some cases. 
    // *Acquire* family of the functions may be used to deduce template parameters
    template<
        // This object must support interface expected by Acquire traits (L parameter)
        typename T,
        // Aqcuire traits. Maps Acquire/Release/TryAcquire calls to object specific
        // method calls
        typename L = AcquireTraits<T> > 
    class ScopedResOwner : private L 
    {
    public:

        typedef L AcquireTraitsT;
        typedef T ResourceT;
        
        ScopedResOwner (ScopedResOwner const &owner) 
        :   resource_(owner.Detach()) 
        {
        }
    
        ScopedResOwner &operator = (ScopedResOwner const &owner) 
        {
            resource_ = owner.Detach();
            return *this;
        }
    
        explicit ScopedResOwner (ResourceT* resource = nullptr) 
        :   resource_(nullptr) 
        { 
            Acquire(resource); 
        }
    
        template <typename P>
        ScopedResOwner (ResourceT* resource, P param) 
        :   resource_(nullptr) 
        { 
            TryAcquire(resource, param); 
        }

        void Acquire (ResourceT* resource) const 
        {
            if (resource != resource_)
            {
                Release();
                if (resource) 
                {
                    AcquireTraitsT::Acquire(resource); 
                    resource_ = resource;
                }
            }
        }
    
        bool IsValid () const 
        {
            return resource_;
        }
    
        bool TryAcquire (ResourceT* resource) const 
        {
            bool rc = false;
            if (resource != resource_ ) 
            {
                Release();
                if (resource) 
                {
                    rc = AcquireTraitsT::TryAcquire(resource);
                    if (rc)
                    {
                        resource_ = resource;    
                    }
                }
            }
            else
            {
                rc = true;
            }
            return rc;
        }
    
        template <typename P>
        bool TryAcquire(ResourceT* resource, P param) const 
        {
            bool rc = false;
            if (resource != resource_ ) 
            {
                Release();
                if (resource) 
                {
                    rc = AcquireTraitsT::TryAcquire(resource, param);
                    if (rc) 
                    {
                        resource_ = resource;    
                    }
                }
            }
            else
            {
                rc = true;
            }
            return rc;
        }
    
        void Attach(ResourceT* resource) 
        {
            if (resource != resource_ ) 
            {
                Release();
                if (resource) 
                {
                    resource_ = resource;
                }
            }
        }
    
        T* Detach() const 
        {
            T *tmp = resource_;
            resource_ = nullptr;
            return tmp;
        }
    
        T* Get() 
        {
            return resource_;
        }
    
        T const * Get() const 
        {
            return resource_;
        }

        void Release() const 
        {
            if (resource_)
            {
                AcquireTraitsT::Release(resource_);
                resource_ = nullptr;
            }
        }

        ~ScopedResOwner() 
        {
            Release();
        }
    
    private:
        mutable ResourceT *resource_;
    };
    
    template<typename T>
    inline
    ScopedResOwner<T, typename T::AcqiureTraitsT> AcquireResource(T &resource) 
    {
        return ScopedResOwner<T, typename T::AcqiureTraitsT>(&resource);
    }
    
    template<typename T>
    inline
    ScopedResOwner<T, typename T::AcqiureTraitsT> TryAcquireResource(T &resource) 
    {
        ScopedResOwner<T, typename T::AcqiureTraitsT> owner;
        owner.TryAcquire(&resource);
        return owner;
    }
    
    template<typename T, typename P>
    inline
    ScopedResOwner<T, typename T::AcqiureTraitsT> TryAcquireResource(T &resource, P param) 
    {
        return ScopedResOwner<T, typename T::AcqiureTraitsT>(&resource, param);
    }
    
    template<typename T>
    inline
    ScopedResOwner<T, typename T::AcqiureSharedTraitsT> AcquireResourceShared(T &resource) 
    {
        return ScopedResOwner<T, typename T::AcqiureSharedTraitsT>(&resource);
    }

    template<typename T>
    inline
    ScopedResOwner<T, typename T::AcqiureExclusiveTraitsT> AcquireResourceExclusive(T &resource) 
    {
        return ScopedResOwner<T, typename T::AcqiureExclusiveTraitsT>(&resource);
    }

} //namespace Common

