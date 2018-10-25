// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;

namespace Common
{
	void TextTraceWriter::operator()(
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format);
			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0);
			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0,
		VariableArgument const & a1) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0,
		VariableArgument const & a1,
		VariableArgument const & a2) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0,
		VariableArgument const & a1,
		VariableArgument const & a2,
		VariableArgument const & a3) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0,
		VariableArgument const & a1,
		VariableArgument const & a2,
		VariableArgument const & a3,
		VariableArgument const & a4) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0,
		VariableArgument const & a1,
		VariableArgument const & a2,
		VariableArgument const & a3,
		VariableArgument const & a4,
		VariableArgument const & a5) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
		StringLiteral type,
		std::wstring const & id,
		StringLiteral format,
		VariableArgument const & a0,
		VariableArgument const & a1,
		VariableArgument const & a2,
		VariableArgument const & a3,
		VariableArgument const & a4,
		VariableArgument const & a5,
		VariableArgument const & a6) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5, a6);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
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
		VariableArgument const & a7) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5, a6, a7);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
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
		VariableArgument const & a8) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5, a6, a7, a8);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
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
		VariableArgument const & a9) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
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
		VariableArgument const & a10) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
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
		VariableArgument const & a11) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
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
		VariableArgument const & a12) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TextTraceWriter::operator()(
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
		VariableArgument const & a13) const
	{
		bool useETW = event_.IsETWSinkEnabled();
		bool useFile = event_.IsFileSinkEnabled();
		bool useConsole = event_.IsConsoleSinkEnabled();

		if (useETW || useFile || useConsole)
		{
			std::wstring text = wformatString(format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);

			event_.WriteTextEvent(type, id, text, useETW, useFile, useConsole);
		}
	}

	void TraceError(
		TraceTaskCodes::Enum taskId,
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
		VariableArgument const & a8)
	{
		TraceProvider::GetSingleton()->GetTextEventWriter(taskId, LogLevel::Error)(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	void TraceError(
		TraceTaskCodes::Enum taskId,
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
		VariableArgument const & a8)
	{
		TraceProvider::GetSingleton()->GetTextEventWriter(taskId, LogLevel::Error)(type, id, format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	void TraceWarning(
		TraceTaskCodes::Enum taskId,
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
		VariableArgument const & a8)
	{
		TraceProvider::GetSingleton()->GetTextEventWriter(taskId, LogLevel::Warning)(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	void TraceWarning(
		TraceTaskCodes::Enum taskId,
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
		VariableArgument const & a8)
	{
		TraceProvider::GetSingleton()->GetTextEventWriter(taskId, LogLevel::Warning)(type, id, format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	void TraceInfo(
		TraceTaskCodes::Enum taskId,
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
		VariableArgument const & a8)
	{
		TraceProvider::GetSingleton()->GetTextEventWriter(taskId, LogLevel::Info)(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	void TraceInfo(
		TraceTaskCodes::Enum taskId,
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
		VariableArgument const & a8)
	{
		TraceProvider::GetSingleton()->GetTextEventWriter(taskId, LogLevel::Info)(type, id, format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	void TraceNoise(
		TraceTaskCodes::Enum taskId,
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
		VariableArgument const & a8)
	{
		TraceProvider::GetSingleton()->GetTextEventWriter(taskId, LogLevel::Noise)(type, std::wstring(), format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	void TraceNoise(
		TraceTaskCodes::Enum taskId,
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
		VariableArgument const & a8)
	{
		TraceProvider::GetSingleton()->GetTextEventWriter(taskId, LogLevel::Noise)(type, id, format, a0, a1, a2, a3, a4, a5, a6, a7, a8);
	}

	//
	// This is a callback that is used by KTL to include its traces
	// into the Service Fabric trace stream. Use
	// KtlSystem::RegisterServiceFabricKtlTraceCallback to pass this to
	// KTL
	//
	VOID ServiceFabricKtlTraceCallback(
		__in LPCWSTR typeText,
		__in USHORT level,
		__in LPCSTR textA)
	{
		wstring parsed_typeText;
		auto parseHResult = StringUtility::LpcwstrToWstring(typeText, false, parsed_typeText);
		if (FAILED(parseHResult))
		{
			return;
		}

		// TODO: Is there a better way to convert TextA. Can we pass that
		// to the Trace apis ?
		std::wstring textW = Common::StringUtility::ToWString(textA);
		LPCWSTR text = textW.c_str();

		std::wstring parsed_text;
		{
			auto parseHResult2 = StringUtility::LpcwstrToWstring(text, false, Common::ParameterValidator::MinStringSize, CommonConfig::GetConfig().MaxLongStringSize, parsed_text); \
				if (FAILED(parseHResult2))
				{
					return;
				}
		}

		LogLevel::Enum logLevel = (LogLevel::Enum)(level);
		std::wstring wsType = parsed_typeText;

		std::string sType(wsType.begin(), wsType.end());

		StringLiteral const type(sType.c_str(), sType.c_str() + sType.length());
		switch (logLevel)
		{
		case LogLevel::Enum::Critical:
		case LogLevel::Enum::Error:
			TraceError(TraceTaskCodes::Ktl, type, "{0}", parsed_text);
			break;
		case LogLevel::Enum::Info:
			TraceInfo(TraceTaskCodes::Ktl, type, "{0}", parsed_text);
			break;
		case LogLevel::Enum::Noise:
			TraceNoise(TraceTaskCodes::Ktl, type, "{0}", parsed_text);
			break;
		case LogLevel::Enum::Warning:
			TraceWarning(TraceTaskCodes::Ktl, type, "{0}", parsed_text);
			break;
		default:
			TraceInfo(TraceTaskCodes::Ktl, type, "{0}", parsed_text);
			break;
		};
	}
}