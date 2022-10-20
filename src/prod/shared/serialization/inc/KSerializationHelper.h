// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "IFabricSerializableStream.h"

namespace Serialization
{
    //template <>
    //struct ArrayUtility<ULONG>
    //{
    //    static NTSTATUS WriteArray(__in IFabricSerializableStream * stream, __in ULONG * data, __in ULONG count)
    //    {
    //        return stream->WriteUInt32Array(data, count);
    //    }
    //
    //    static NTSTATUS ReadArray(__in IFabricSerializableStream * stream, __out ULONG * data, __inout ULONG & count)
    //    {
    //        return stream->ReadUInt32Array(data, count);
    //    }
    //};

    template <class T>
    IFabricSerializable * DefaultActivator(FabricTypeInformation)
    {
        return _new(WF_SERIALIZATION_TAG, FabricSerializationCommon::GetNonPagedKtlAllocator()) T();
    }

    struct KSerializationHelper
    {
        KSerializationHelper(KSerializationHelper const &);
        KSerializationHelper & operator=(KSerializationHelper const &);

    public:
        KSerializationHelper(IFabricSerializableStream * stream)
            : stream_(stream)
        {
        }

        template<class T>
        NTSTATUS Write(T & object)
        {
            __if_exists(T::GetTypeInformation)
            {
                return this->stream_->WriteSerializable(&object);
            }
            __if_not_exists(T::GetTypeInformation)
            {
                static_assert(false, "This is a T");
            }
        }

        template<class T>
        NTSTATUS Read(T & object)
        {
            __if_exists(T::GetTypeInformation)
            {
                return this->stream_->ReadSerializable(&object);
            }
            __if_not_exists(T::GetTypeInformation)
            {
                static_assert(false, "This is a T");
            }
        }

        NTSTATUS Write(IFabricSerializable * object)
        {
            return this->stream_->WritePointer(object);
        }

        NTSTATUS Read(__out IFabricSerializable ** object, __in FabricActivatorFunction activator)
        {
            return this->stream_->ReadPointer(object, activator);
        }

        template <class T>
        NTSTATUS Write(KUniquePtr<T> & object)
        {
            return this->stream_->WritePointer(&(*object));
        }

        template <class T>
        NTSTATUS Read(KUniquePtr<T> & object, __in FabricActivatorFunction activator = nullptr)
        {
            IFabricSerializable * pointer = nullptr;

            if (activator == nullptr)
            {
                activator = DefaultActivator<T>;
            }

            NTSTATUS status = this->stream_->ReadPointer(&pointer, activator);

            if (NT_SUCCESS(status))
            {
                object = KUniquePtr<T>(static_cast<T*>(pointer));
            }

            return status;
        }

        NTSTATUS Write(IFabricSerializable & object)
        {
            return this->stream_->WriteSerializable(&object);
        }

        NTSTATUS Read(IFabricSerializable & object)
        {
            return this->stream_->ReadSerializable(&object);
        }

        NTSTATUS Write(KWString & field)
        { 
            UNICODE_STRING * unicodeField = static_cast<UNICODE_STRING*>(field);

            return this->stream_->WriteString(unicodeField->Buffer, unicodeField->Length / 2);
        }

        NTSTATUS Read(KWString & field)
        {
            field = KWString(FabricSerializationCommon::GetNonPagedKtlAllocator());

            if (NT_ERROR(field.Status()))
            {
                return field.Status();
            }

            ULONG size = 0;
            WCHAR * buffer;
            KUniquePtr<WCHAR> bufferUPtr;

            NTSTATUS status = this->stream_->ReadString(nullptr, size);

            if (K_STATUS_BUFFER_TOO_SMALL == status)
            {
                ULONG allocSize;
                status = ULongAdd(size, 1, &allocSize);
                if (!NT_SUCCESS(status)) return status;

                buffer = _new(WF_SERIALIZATION_TAG, FabricSerializationCommon::GetNonPagedKtlAllocator()) WCHAR[allocSize];

                if (buffer == nullptr)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                bufferUPtr = KUniquePtr<WCHAR>(buffer);
            }
            else if (NT_SUCCESS(status))
            {
                return STATUS_SUCCESS;
            }
            else
            {
                return status;
            }

            status = this->stream_->ReadString(buffer, size);

            if (NT_SUCCESS(status))
            {
                buffer[size] = L'\0';
            }
            else
            {
                return status;
            }

            field = KWString(FabricSerializationCommon::GetNonPagedKtlAllocator(), buffer);

            if (NT_ERROR(field.Status()))
            {
                return field.Status();
            }

            return STATUS_SUCCESS;
        }

        template <class T>
        NTSTATUS Write(KArray<KUniquePtr<T>> & field)
        {
            static_assert(sizeof(T*) == sizeof(KUniquePtr<T>), "Size of T pointer does not equal the size of KUniquePtr<T>, cast isn't safe");

            // TODO: this relies on the above assertion that a KUniquePtr is really just a pointer
            IFabricSerializable ** pointerArray = reinterpret_cast<IFabricSerializable**>(field.Data());

            return this->stream_->WritePointerArray(pointerArray, field.Count());
        }

        template <class T>
        NTSTATUS Read(KArray<KUniquePtr<T>> & field, __in FabricActivatorFunction activator = nullptr)
        {
            if (activator == nullptr)
            {
                activator = DefaultActivator<T>;
            }

            static_assert(sizeof(T*) == sizeof(KUniquePtr<T>), "Size of T pointer does not equal the size of KUniquePtr<T>, cast isn't safe");

            // TODO: this relies on the above assertion that a KUniquePtr is really just a pointer
            IFabricSerializable ** pointerArray = reinterpret_cast<IFabricSerializable**>(field.Data());

            ULONG size = field.Max();
            NTSTATUS status = this->stream_->ReadPointerArray(pointerArray, activator, size);

            if (K_STATUS_BUFFER_TOO_SMALL == status)
            {
                field = KArray<KUniquePtr<T>>(FabricSerializationCommon::GetNonPagedKtlAllocator(), size, size, 50);  // Create a new array with correct size
                status = field.Status();

                if (NT_ERROR(status))
                {
                    return status;
                }
            }
            else if (NT_SUCCESS(status))
            {
                BOOL setCountBool = FALSE;
                setCountBool = field.SetCount(size);
                ASSERT(setCountBool == TRUE);

                return STATUS_SUCCESS;
            }
            else
            {
                return status;
            }

            size = field.Max();

            // Make sure we are pointing to new buffer
            pointerArray = reinterpret_cast<IFabricSerializable**>(field.Data());

            status = this->stream_->ReadPointerArray(pointerArray, activator, size);

            if (NT_ERROR(status))
            {
                return status;
            }

            return status;
        }

        template <class T>
        NTSTATUS Write(KArray<T> & field)
        {
            return ArrayUtility<T>::WriteArray(this->stream_, field.Data(), field.Count());
        }

        template <class T>
        NTSTATUS Read(KArray<T> & field)
        {
            ULONG size = field.Max();
            NTSTATUS status = ArrayUtility<T>::ReadArray(this->stream_, field.Data(), size);

            if (K_STATUS_BUFFER_TOO_SMALL == status)
            {
                field = KArray<T>(FabricSerializationCommon::GetNonPagedKtlAllocator(), size, size, 50);  // Create a new array with correct size
                status = field.Status();

                if (NT_ERROR(status))
                {
                    return status;
                }
            }
            else if (NT_SUCCESS(status))
            {
                BOOL setCountBool = FALSE;
                setCountBool = field.SetCount(size);
                ASSERT(setCountBool == TRUE);

                return STATUS_SUCCESS;
            }
            else
            {
                return status;
            }

            size = field.Max();

            status = ArrayUtility<T>::ReadArray(this->stream_, field.Data(), size);

            if (NT_ERROR(status))
            {
                return status;
            }

            return status;
        }

        NTSTATUS WriteStartType() { return this->stream_->WriteStartType(); };

        NTSTATUS WriteEndType(FabricCompletionCallback callback = nullptr, VOID * state = nullptr) { return this->stream_->WriteEndType(callback, state); };

        NTSTATUS Write(bool field) { return this->stream_->WriteBool(field); };

        NTSTATUS Write(CHAR field) { return this->stream_->WriteChar(field); };

        NTSTATUS Write(UCHAR field) { return this->stream_->WriteUChar(field); };

        NTSTATUS Write(SHORT field) { return this->stream_->WriteShort(field); };

        NTSTATUS Write(USHORT field) { return this->stream_->WriteUShort(field); };

        NTSTATUS Write(LONG field) { return this->stream_->WriteInt32(field); };

        NTSTATUS Write(ULONG field) { return this->stream_->WriteUInt32(field); };

        NTSTATUS Write(LONG64 field) { return this->stream_->WriteInt64(field); };

        NTSTATUS Write(ULONG64 field) { return this->stream_->WriteUInt64(field); };

        NTSTATUS Write(GUID field) { return this->stream_->WriteGuid(field); };

        NTSTATUS Write(KGuid field) { return this->stream_->WriteGuid(field); };

        NTSTATUS Write(DOUBLE field) { return this->stream_->WriteDouble(field); };

        NTSTATUS ReadStartType() { return this->stream_->ReadStartType(); };

        NTSTATUS ReadEndType() { return this->stream_->ReadEndType(); };

        NTSTATUS Read(bool & field) { return this->stream_->ReadBool(field); };

        NTSTATUS Read(CHAR & field) { return this->stream_->ReadChar(field); };

        NTSTATUS Read(UCHAR & field) { return this->stream_->ReadUChar(field); };

        NTSTATUS Read(SHORT & field) { return this->stream_->ReadShort(field); };

        NTSTATUS Read(USHORT & field) { return this->stream_->ReadUShort(field); };

        NTSTATUS Read(LONG & field) { return this->stream_->ReadInt32(field); };

        NTSTATUS Read(ULONG & field) { return this->stream_->ReadUInt32(field); };

        NTSTATUS Read(LONG64 & field) { return this->stream_->ReadInt64(field); };

        NTSTATUS Read(ULONG64 & field) { return this->stream_->ReadUInt64(field); };

        NTSTATUS Read(GUID & field) { return this->stream_->ReadGuid(field); };

        NTSTATUS Read(KGuid & field) { return this->stream_->ReadGuid(field); };

        NTSTATUS Read(DOUBLE & field) { return this->stream_->ReadDouble(field); };

    private:

        //template <class T>
        //static ActivatorFunction GetActivatorFunction()
        //{
        //    __if_exists(T::Create)
        //    {
        //        return T::Create;
        //    }

        //    __if_not_exists(T::Create)
        //    {
        //        return ActivatorHelper<T>::Create;
        //    }
        //}

        IFabricSerializableStream * stream_;
    };


    // Below code was generated by a tool

    #define CheckStatus(status)         \
        if (NT_ERROR(status))           \
        {                               \
            return status;              \
        }                               \


    #define K_FIELDS_01(ARG0) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_02(ARG0, ARG1) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_03(ARG0, ARG1, ARG2) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_04(ARG0, ARG1, ARG2, ARG3) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_05(ARG0, ARG1, ARG2, ARG3, ARG4) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_06(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_07(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_08(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_09(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_10(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_11(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_12(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_13(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_14(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_15(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_16(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_17(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_18(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG17);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG17);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_19(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG17);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG18);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG17);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG18);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_20(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG17);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG18);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG19);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG17);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG18);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG19);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_21(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG17);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG18);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG19);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG20);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG17);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG18);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG19);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG20);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_22(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG17);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG18);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG19);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG20);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG21);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG17);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG18);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG19);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG20);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG21);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_23(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG17);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG18);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG19);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG20);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG21);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG22);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG17);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG18);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG19);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG20);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG21);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG22);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_24(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG17);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG18);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG19);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG20);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG21);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG22);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG23);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG17);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG18);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG19);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG20);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG21);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG22);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG23);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \


    #define K_FIELDS_25(ARG0, ARG1, ARG2, ARG3, ARG4, ARG5, ARG6, ARG7, ARG8, ARG9, ARG10, ARG11, ARG12, ARG13, ARG14, ARG15, ARG16, ARG17, ARG18, ARG19, ARG20, ARG21, ARG22, ARG23, ARG24) \
        virtual NTSTATUS Write(IFabricSerializableStream * stream)       \
        {                                                           \
            NTSTATUS status = __super::Write(stream);               \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.WriteStartType();                 \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG0);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG1);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG2);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG3);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG4);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG5);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG6);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG7);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG8);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG9);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG10);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG11);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG12);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG13);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG14);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG15);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG16);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG17);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG18);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG19);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG20);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG21);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG22);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG23);                      \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Write(ARG24);                      \
            CheckStatus(status);                                    \
                                                                    \
            return streamHelper.WriteEndType();                     \
        }                                                           \
                                                                    \
        virtual NTSTATUS Read(IFabricSerializableStream * stream)        \
        {                                                           \
            NTSTATUS status = __super::Read(stream);                \
            CheckStatus(status);                                    \
                                                                    \
            KSerializationHelper streamHelper(stream);              \
                                                                    \
            status = streamHelper.ReadStartType();                  \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG0);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG1);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG2);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG3);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG4);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG5);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG6);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG7);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG8);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG9);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG10);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG11);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG12);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG13);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG14);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG15);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG16);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG17);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG18);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG19);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG20);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG21);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG22);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG23);                       \
            CheckStatus(status);                                    \
                                                                    \
            status = streamHelper.Read(ARG24);                       \
            CheckStatus(status);                                    \
                                                                    \
            return stream->ReadEndType();                           \
        }                                                           \

}

