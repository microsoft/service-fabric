#pragma once
#if defined(K_UseResumable)

namespace ktl
{
	namespace io
	{
		interface KStream
		{
			K_SHARED_INTERFACE(KStream);

		public:

            virtual Awaitable<NTSTATUS> CloseAsync() = 0;

            //
            //  Read "Count" bytes from the stream into "Buffer" starting at buffer offset "OffsetIntoBuffer"
            //
            //  Parameters:
            //      Buffer - The buffer to read into.
            //      BytesRead - The total number of bytes read into Buffer.  If a read is requested at or beyond the end of the stream, BytesRead will be zero.
            //      OffsetIntoBuffer - The offset into Buffer to read into.
            //      Count - The number of bytes to read.  If 0, a length of Buffer.QuerySize() - Count will be used.
            //      
			virtual Awaitable<NTSTATUS> ReadAsync(
				__in KBuffer& Buffer,
                __out ULONG& BytesRead,
				__in ULONG OffsetIntoBuffer = 0,
                __in ULONG Count = 0
			) = 0;

            //
            //  Write "Count" bytes to the stream from "Buffer" starting at buffer offset "OffsetIntoBuffer"
            //
            //  Parameters:
            //      Buffer - The buffer to write from.
            //      OffsetIntoBuffer - The offset into Buffer to write from.
            //      Count - The number of bytes to write.  If 0, a length of Buffer.QuerySize() - Count will be used.
            //  
			virtual Awaitable<NTSTATUS> WriteAsync(
				__in KBuffer const & Buffer,
                __in ULONG OffsetIntoBuffer = 0,
                __in ULONG Count = 0
			) = 0;

            //
            //  If necessary, flush any buffered writes to disk.
            //
            virtual Awaitable<NTSTATUS> FlushAsync() = 0;

            //
            // The current position of the stream.
            //
            __declspec(property(get = GetPosition, put = SetPosition)) LONGLONG Position;
            virtual LONGLONG GetPosition() const = 0;
            virtual void SetPosition(__in LONGLONG Position) = 0;

            //
            // The current length of the stream.
            //
            __declspec(property(get = GetLength)) LONGLONG Length;
            virtual LONGLONG GetLength() const = 0;
		};
	}
}

#endif
