// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{

	namespace detail
	{
		struct format_handler
		{
			template <typename T>
			std::string operator () (T const & t) const
			{
				std::string result;
				StringWriterA w(result);
				w.Write(t);
				return result;
			}

			std::string operator () (
				StringLiteral format,
				VariableArgument const & arg0,
				VariableArgument const & arg1 = VariableArgument(),
				VariableArgument const & arg2 = VariableArgument(),
				VariableArgument const & arg3 = VariableArgument(),
				VariableArgument const & arg4 = VariableArgument(),
				VariableArgument const & arg5 = VariableArgument(),
				VariableArgument const & arg6 = VariableArgument(),
				VariableArgument const & arg7 = VariableArgument(),
				VariableArgument const & arg8 = VariableArgument(),
				VariableArgument const & arg9 = VariableArgument(),
				VariableArgument const & arg10 = VariableArgument(),
				VariableArgument const & arg11 = VariableArgument(),
				VariableArgument const & arg12 = VariableArgument(),
				VariableArgument const & arg13 = VariableArgument()) const;
		};

		struct wformat_handler
		{
			template <typename T>
			std::wstring operator () (T const & t) const
			{
				std::wstring result;
				StringWriter w(result);
				w.Write(t);
				return result;
			}

			std::wstring operator () (
				StringLiteral format,
				VariableArgument const & arg0,
				VariableArgument const & arg1 = VariableArgument(),
				VariableArgument const & arg2 = VariableArgument(),
				VariableArgument const & arg3 = VariableArgument(),
				VariableArgument const & arg4 = VariableArgument(),
				VariableArgument const & arg5 = VariableArgument(),
				VariableArgument const & arg6 = VariableArgument(),
				VariableArgument const & arg7 = VariableArgument(),
				VariableArgument const & arg8 = VariableArgument(),
				VariableArgument const & arg9 = VariableArgument(),
				VariableArgument const & arg10 = VariableArgument(),
				VariableArgument const & arg11 = VariableArgument(),
				VariableArgument const & arg12 = VariableArgument(),
				VariableArgument const & arg13 = VariableArgument()) const;

			std::wstring operator () (
				std::wstring const & wformat,
				VariableArgument const & arg0,
				VariableArgument const & arg1 = VariableArgument(),
				VariableArgument const & arg2 = VariableArgument(),
				VariableArgument const & arg3 = VariableArgument(),
				VariableArgument const & arg4 = VariableArgument(),
				VariableArgument const & arg5 = VariableArgument(),
				VariableArgument const & arg6 = VariableArgument(),
				VariableArgument const & arg7 = VariableArgument(),
				VariableArgument const & arg8 = VariableArgument(),
				VariableArgument const & arg9 = VariableArgument(),
				VariableArgument const & arg10 = VariableArgument(),
				VariableArgument const & arg11 = VariableArgument(),
				VariableArgument const & arg12 = VariableArgument(),
				VariableArgument const & arg13 = VariableArgument()) const;
		};
	}
	extern detail::format_handler & formatString;
	extern detail::wformat_handler & wformatString;

	class TextTraceWriter
	{
		DENY_COPY_ASSIGNMENT(TextTraceWriter);

	public:
		TextTraceWriter(TraceEvent & textEvent)
			: event_(textEvent)
		{
		}

		TextTraceWriter(TraceTaskCodes::Enum taskId, LogLevel::Enum level)
			: event_(TraceProvider::StaticInit_Singleton()->GetTextEvent(taskId, level))
		{
		}

		void operator()(
			StringLiteral type,
			StringLiteral format) const
		{
			operator()(type, format, std::wstring());
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0) const
		{
			bool useETW = event_.IsETWSinkEnabled();
			bool useFile = event_.IsFileSinkEnabled();
			bool useConsole = event_.IsConsoleSinkEnabled();

			if (useETW || useFile || useConsole)
			{
				std::wstring text = wformatString(format, a0);

				event_.WriteTextEvent(type, std::wstring(), text, useETW, useFile, useConsole);
			}
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1) const
		{
			operator()(type, std::wstring(), format, a0, a1);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9,
			VariableArgument const & a10) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9,
			VariableArgument const & a10,
			VariableArgument const & a11) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9,
			VariableArgument const & a10,
			VariableArgument const & a11,
			VariableArgument const & a12) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
		}

		void operator()(
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9,
			VariableArgument const & a10,
			VariableArgument const & a11,
			VariableArgument const & a12,
			VariableArgument const & a13) const
		{
			operator()(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
		}

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9,
			VariableArgument const & a10) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9,
			VariableArgument const & a10,
			VariableArgument const & a11) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9,
			VariableArgument const & a10,
			VariableArgument const & a11,
			VariableArgument const & a12) const;

		void operator()(
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0,
			VariableArgument const & a1,
			VariableArgument const & a2,
			VariableArgument const & a3,
			VariableArgument const & a4,
			VariableArgument const & a5,
			VariableArgument const & a6,
			VariableArgument const & a7,
			VariableArgument const & a8,
			VariableArgument const & a9,
			VariableArgument const & a10,
			VariableArgument const & a11,
			VariableArgument const & a12,
			VariableArgument const & a13) const;

	private:
		TraceEvent & event_;
	};

	class TextLevelTraceWriter
	{
		DENY_COPY(TextLevelTraceWriter);

	public:
		TextLevelTraceWriter(TraceTaskCodes::Enum taskId)
			: errorEvent_(TraceProvider::StaticInit_Singleton()->GetTextEventWriter(taskId, LogLevel::Error)),
			warningEvent_(TraceProvider::StaticInit_Singleton()->GetTextEventWriter(taskId, LogLevel::Warning)),
			infoEvent_(TraceProvider::StaticInit_Singleton()->GetTextEventWriter(taskId, LogLevel::Info)),
			noiseEvent_(TraceProvider::StaticInit_Singleton()->GetTextEventWriter(taskId, LogLevel::Noise))
		{
		}

		void operator()(
			LogLevel::Enum level,
			StringLiteral type,
			StringLiteral format,
			VariableArgument const & a0 = VariableArgument(),
			VariableArgument const & a1 = VariableArgument(),
			VariableArgument const & a2 = VariableArgument(),
			VariableArgument const & a3 = VariableArgument(),
			VariableArgument const & a4 = VariableArgument(),
			VariableArgument const & a5 = VariableArgument(),
			VariableArgument const & a6 = VariableArgument(),
			VariableArgument const & a7 = VariableArgument(),
			VariableArgument const & a8 = VariableArgument()) const
		{
			switch (level)
			{
			case LogLevel::Error: errorEvent_(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8); break;
			case LogLevel::Warning: warningEvent_(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8); break;
			case LogLevel::Info: infoEvent_(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8); break;
			case LogLevel::Noise: noiseEvent_(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8); break;
			}
		}

		void operator()(
			LogLevel::Enum level,
			StringLiteral type,
			std::wstring const & id,
			StringLiteral format,
			VariableArgument const & a0 = VariableArgument(),
			VariableArgument const & a1 = VariableArgument(),
			VariableArgument const & a2 = VariableArgument(),
			VariableArgument const & a3 = VariableArgument(),
			VariableArgument const & a4 = VariableArgument(),
			VariableArgument const & a5 = VariableArgument(),
			VariableArgument const & a6 = VariableArgument(),
			VariableArgument const & a7 = VariableArgument(),
			VariableArgument const & a8 = VariableArgument()) const
		{
			switch (level)
			{
			case LogLevel::Error: errorEvent_(type, id, format, a0, a1, a2, a3, a4, a5, a6, a7, a8); break;
			case LogLevel::Warning: warningEvent_(type, id, format, a0, a1, a2, a3, a4, a5, a6, a7, a8); break;
			case LogLevel::Info: infoEvent_(type, id, format, a0, a1, a2, a3, a4, a5, a6, a7, a8); break;
			case LogLevel::Noise: noiseEvent_(type, id, format, a0, a1, a2, a3, a4, a5, a6, a7, a8); break;
			}
		}

	private:
		TextTraceWriter errorEvent_;
		TextTraceWriter warningEvent_;
		TextTraceWriter infoEvent_;
		TextTraceWriter noiseEvent_;
	};


#pragma region ( TextTraceComponent )
	template <TraceTaskCodes::Enum id>
	class TextTraceComponent
	{
	public:
		static const TextTraceWriter WriteError;
		static const TextTraceWriter WriteWarning;
		static const TextTraceWriter WriteInfo;
		static const TextTraceWriter WriteNoise;
		static const TextLevelTraceWriter WriteTrace;
	};

	template <TraceTaskCodes::Enum id> const TextTraceWriter TextTraceComponent<id>::WriteError(id, LogLevel::Error);
	template <TraceTaskCodes::Enum id> const TextTraceWriter TextTraceComponent<id>::WriteWarning(id, LogLevel::Warning);
	template <TraceTaskCodes::Enum id> const TextTraceWriter TextTraceComponent<id>::WriteInfo(id, LogLevel::Info);
	template <TraceTaskCodes::Enum id> const TextTraceWriter TextTraceComponent<id>::WriteNoise(id, LogLevel::Noise);
	template <TraceTaskCodes::Enum id> const TextLevelTraceWriter TextTraceComponent<id>::WriteTrace(id);
#pragma endregion

#pragma region ( TraceError )

	void TraceError(
		TraceTaskCodes::Enum taskId,
		StringLiteral type,
		StringLiteral format,
		VariableArgument const & a0 = VariableArgument(),
		VariableArgument const & a1 = VariableArgument(),
		VariableArgument const & a2 = VariableArgument(),
		VariableArgument const & a3 = VariableArgument(),
		VariableArgument const & a4 = VariableArgument(),
		VariableArgument const & a5 = VariableArgument(),
		VariableArgument const & a6 = VariableArgument(),
		VariableArgument const & a7 = VariableArgument(),
		VariableArgument const & a8 = VariableArgument());

	void TraceError(
		TraceTaskCodes::Enum taskId,
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0 = VariableArgument(),
		VariableArgument const & a1 = VariableArgument(),
		VariableArgument const & a2 = VariableArgument(),
		VariableArgument const & a3 = VariableArgument(),
		VariableArgument const & a4 = VariableArgument(),
		VariableArgument const & a5 = VariableArgument(),
		VariableArgument const & a6 = VariableArgument(),
		VariableArgument const & a7 = VariableArgument(),
		VariableArgument const & a8 = VariableArgument());
#pragma endregion

#pragma region ( TraceWarning )
	void TraceWarning(
		TraceTaskCodes::Enum taskId,
		StringLiteral type,
		StringLiteral format,
		VariableArgument const & a0 = VariableArgument(),
		VariableArgument const & a1 = VariableArgument(),
		VariableArgument const & a2 = VariableArgument(),
		VariableArgument const & a3 = VariableArgument(),
		VariableArgument const & a4 = VariableArgument(),
		VariableArgument const & a5 = VariableArgument(),
		VariableArgument const & a6 = VariableArgument(),
		VariableArgument const & a7 = VariableArgument(),
		VariableArgument const & a8 = VariableArgument());

	void TraceWarning(
		TraceTaskCodes::Enum taskId,
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0 = VariableArgument(),
		VariableArgument const & a1 = VariableArgument(),
		VariableArgument const & a2 = VariableArgument(),
		VariableArgument const & a3 = VariableArgument(),
		VariableArgument const & a4 = VariableArgument(),
		VariableArgument const & a5 = VariableArgument(),
		VariableArgument const & a6 = VariableArgument(),
		VariableArgument const & a7 = VariableArgument(),
		VariableArgument const & a8 = VariableArgument());
#pragma endregion

#pragma region ( TraceInfo )
	void TraceInfo(
		TraceTaskCodes::Enum taskId,
		StringLiteral type,
		StringLiteral format,
		VariableArgument const & a0 = VariableArgument(),
		VariableArgument const & a1 = VariableArgument(),
		VariableArgument const & a2 = VariableArgument(),
		VariableArgument const & a3 = VariableArgument(),
		VariableArgument const & a4 = VariableArgument(),
		VariableArgument const & a5 = VariableArgument(),
		VariableArgument const & a6 = VariableArgument(),
		VariableArgument const & a7 = VariableArgument(),
		VariableArgument const & a8 = VariableArgument());

	void TraceInfo(
		TraceTaskCodes::Enum taskId,
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0 = VariableArgument(),
		VariableArgument const & a1 = VariableArgument(),
		VariableArgument const & a2 = VariableArgument(),
		VariableArgument const & a3 = VariableArgument(),
		VariableArgument const & a4 = VariableArgument(),
		VariableArgument const & a5 = VariableArgument(),
		VariableArgument const & a6 = VariableArgument(),
		VariableArgument const & a7 = VariableArgument(),
		VariableArgument const & a8 = VariableArgument());
#pragma endregion

#pragma region ( TraceNoise )
	void TraceNoise(
		TraceTaskCodes::Enum taskId,
		StringLiteral type,
		StringLiteral format,
		VariableArgument const & a0 = VariableArgument(),
		VariableArgument const & a1 = VariableArgument(),
		VariableArgument const & a2 = VariableArgument(),
		VariableArgument const & a3 = VariableArgument(),
		VariableArgument const & a4 = VariableArgument(),
		VariableArgument const & a5 = VariableArgument(),
		VariableArgument const & a6 = VariableArgument(),
		VariableArgument const & a7 = VariableArgument(),
		VariableArgument const & a8 = VariableArgument());

	void TraceNoise(
		TraceTaskCodes::Enum taskId,
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0 = VariableArgument(),
		VariableArgument const & a1 = VariableArgument(),
		VariableArgument const & a2 = VariableArgument(),
		VariableArgument const & a3 = VariableArgument(),
		VariableArgument const & a4 = VariableArgument(),
		VariableArgument const & a5 = VariableArgument(),
		VariableArgument const & a6 = VariableArgument(),
		VariableArgument const & a7 = VariableArgument(),
		VariableArgument const & a8 = VariableArgument());
#pragma endregion

	VOID ServiceFabricKtlTraceCallback(
		__in LPCWSTR typeText,
		__in USHORT level,
		__in LPCSTR textA);

}

extern Common::TextTraceComponent<Common::TraceTaskCodes::General> Trace;
