// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <class TComInterface>
    class ComPointer
    {
    public:
        typedef TComInterface InterfaceType;

        // Construction from raw pointer is intentionally disallowed because it is not clear
        // whether the caller wants ComPointer to AddRef or not.  To make this explicit, we
        // require the caller to construct an empty ComPointer and call SetNoAddRef or
        // SetAndAddRef explicitly.
        //
        // However, note that there is a constructor that does QI.  Because the lifetime model
        // for QI is explicit this is safe.

        explicit ComPointer() throw() : p_(NULL) { }

        ComPointer(ComPointer<TComInterface> const & other) throw()
            : p_(other.p_)
        {
            if (p_) { p_->AddRef(); }
        }

        ComPointer(ComPointer<TComInterface> && other) throw()
            : p_(other.p_)
        {
            other.p_ = NULL;
        }

        // Supports "casting" a smart-pointer via QueryInterface.  This intentionally does not
        // make riid an optional parameter so that the mandatory REFIID parameter gives a hint
        // to the caller that a QueryInterface is occurring.
        template <class TOtherComInterface>
        ComPointer(ComPointer<TOtherComInterface> const & other, REFIID riid) throw()
            : p_(NULL)
        {
            CODING_ERROR_ASSERT(other);
            if (FAILED(other->QueryInterface(riid, reinterpret_cast<void**>(&p_))))
            {
                // Correct implementations of QI will always set this NULL, but there are many
                // incorrect implementations of QI in the world.
                p_ = NULL;
            }
        }

        // Similar to previous constructor, but uses IUnknown*.
        ComPointer(IUnknown * other, REFIID riid) throw()
            : p_(NULL)
        {
            CODING_ERROR_ASSERT(other != NULL);
            if (FAILED(other->QueryInterface(riid, reinterpret_cast<void**>(&p_))))
            {
                p_ = NULL;
            }
        }

        ~ComPointer() throw() { if (p_) { p_->Release(); } }

        // --------------------------------------------------------------------
        // Safe operators and methods

        bool operator == (ComPointer const & other) const throw() { return p_ == other.p_; }
        bool operator != (ComPointer const & other) const throw() { return p_ != other.p_; }
        TComInterface * operator -> () const throw() { return p_; }

        ComPointer<TComInterface> const & operator = (ComPointer<TComInterface> const & other) throw()
        {
            if (p_ != other.p_)
            {
                ComPointer<TComInterface> cleanup(std::move(*this));
                p_ = other.p_;
                if (p_) { p_->AddRef(); }
            }
            return *this;
        }

        ComPointer<TComInterface> const & operator = (ComPointer<TComInterface> && other) throw()
        {
            if (this != &other)
            {
                ComPointer<TComInterface> cleanup(std::move(*this));
                p_ = other.p_;
                other.p_ = NULL;
            }
            return *this;
        }

        void Swap(__in ComPointer<TComInterface> & other) throw()
        {
            TComInterface * p = p_;
            p_ = other.p_;
            other.p_ = p;
        }

        void Release() throw()
        {
            ComPointer<TComInterface> cleanup(std::move(*this));
        }

        template<class TOtherComInterface>
        bool IsSameComObject(ComPointer<TOtherComInterface> const & other) const throw()
        {
            if (!(*this))
            {
                return !other;
            }
            else if (!other)
            {
                return false;
            }
            else
            {
                ComPointer<IUnknown> unknownThis(*this, IID_IUnknown);
                ComPointer<IUnknown> unknownOther(other, IID_IUnknown);
                return unknownThis == unknownOther;
            }
        }

        // --------------------------------------------------------------------
        // Raw initialization methods.
        // These are the only ways to get a raw COM pointer in to this class,
        // other than the casting constructor that takes IUnknown.
        //

        TComInterface ** InitializationAddress() throw()
        {
            // This method can only be used to initialize an empty ComPointer.
            CODING_ERROR_ASSERT(p_ == NULL);
            return &p_;
        }

        void ** VoidInitializationAddress() throw()
        {
            // This method can only be used to initialize an empty ComPointer.
            CODING_ERROR_ASSERT(p_ == NULL);
            return reinterpret_cast<void **>(&p_);
        }

        void SetNoAddRef(__in_opt TComInterface * p) throw()
        {
            if (p != p_)
            {
                ComPointer<TComInterface> cleanup(std::move(*this));
                p_ = p;
            }
        }

        void SetAndAddRef(__in_opt TComInterface * p) throw()
        {
            if (p != p_)
            {
                ComPointer<TComInterface> cleanup(std::move(*this));
                p_ = p;
                if (p_) { p_->AddRef(); }
            }
        }

        // --------------------------------------------------------------------
        // Raw detach method.
        // These are the only way to get a raw COM pointer out of this class.
        //

        TComInterface * DetachNoRelease() throw()
        {
            TComInterface * p = p_;
            p_ = NULL;
            return p;
        }

        TComInterface * GetRawPointer() const throw()
        {
            return p_;
        }

    private:
        TComInterface * p_;

    public: // keep intellisense happy by moving this to the bottom
        OPERATOR_BOOL { return (p_ != NULL); }
    };
}
