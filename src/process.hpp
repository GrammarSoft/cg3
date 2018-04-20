/*
* Copyright (C) 2007-2018, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com>
* Design by Eckhard Bick <eckhard.bick@mail.dk>, Tino Didriksen <mail@tinodidriksen.com>
*
* This file is part of VISL CG-3
*
* VISL CG-3 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* VISL CG-3 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with VISL CG-3.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#ifndef c6d28b7452ec699b_PROCESS_HPP
#define c6d28b7452ec699b_PROCESS_HPP
#include <string>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>

class Process {
public:
	Process() :
	g_hChildStd_IN_Rd(0),
	g_hChildStd_IN_Wr(0),
	g_hChildStd_OUT_Rd(0),
	g_hChildStd_OUT_Wr(0) {
	}

	~Process() {
		CloseHandle(g_hChildStd_IN_Rd);
		CloseHandle(g_hChildStd_IN_Wr);
		CloseHandle(g_hChildStd_OUT_Rd);
		CloseHandle(g_hChildStd_OUT_Wr);
	}

	void start(const std::string& cmdline) {
		SECURITY_ATTRIBUTES saAttr = { sizeof(saAttr), 0, true };

		if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0)) {
			std::string msg = formatLastError("Process CreatePipe 1");
			throw std::runtime_error(msg);
		}
		if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
			std::string msg = formatLastError("Process SetHandleInformation 1");
			throw std::runtime_error(msg);
		}
		if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0)) {
			std::string msg = formatLastError("Process CreatePipe 2");
			throw std::runtime_error(msg);
		}
		if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0)) {
			std::string msg = formatLastError("Process SetHandleInformation 2");
			throw std::runtime_error(msg);
		}

		PROCESS_INFORMATION piProcInfo = { 0 };
		STARTUPINFOA siStartInfo = { sizeof(siStartInfo) };

		siStartInfo.hStdError = g_hChildStd_OUT_Wr;
		siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
		siStartInfo.hStdInput = g_hChildStd_IN_Rd;
		siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

		BOOL bSuccess = CreateProcessA(0,
			const_cast<char*>(cmdline.c_str()),
			0,
			0,
			TRUE,
			CREATE_NO_WINDOW | BELOW_NORMAL_PRIORITY_CLASS,
			0,
			0,
			&siStartInfo,
			&piProcInfo);

		if (!bSuccess) {
			std::string msg("Process could not start!\nCmdline: ");
			msg += cmdline.c_str();
			msg += '\n';
			msg = formatLastError(msg);
			throw std::runtime_error(msg);
		}

		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	}

	void read(char *buffer, size_t count) {
		DWORD bytes_read = 0;
		if (!ReadFile(g_hChildStd_OUT_Rd, buffer, count, &bytes_read, 0) || bytes_read != count) {
			std::string msg = formatLastError("Process.read(char*,size_t)");
			throw std::runtime_error(msg);
		}
	}

	void write(const char *buffer, size_t length) {
		DWORD bytes = 0;
		if (!WriteFile(g_hChildStd_IN_Wr, buffer, length, &bytes, 0) || bytes != length) {
			std::string msg = formatLastError("Process.write(char*,size_t)");
			throw std::runtime_error(msg);
		}
	}

	void flush() {
	}

private:
	HANDLE g_hChildStd_IN_Rd;
	HANDLE g_hChildStd_IN_Wr;
	HANDLE g_hChildStd_OUT_Rd;
	HANDLE g_hChildStd_OUT_Wr;

	std::string formatLastError(std::string msg = "") {
		if (!msg.empty()) {
			msg += ' ';
		}
		char *fmt = 0;
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, 0, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPSTR)&fmt, 0, 0);
		msg += "GetLastError: ";
		msg += fmt;
		msg += '\n';
		LocalFree(fmt);
		return msg;
	}
};

#else
#include <popen_plus.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

class Process {
private:
	popen_plus_process *child;

	std::string formatLastError(std::string msg = "") {
		if (!msg.empty()) {
			msg += ' ';
		}
		msg += "strerror: ";
		msg += strerror(errno);
		return msg;
	}

public:

	Process() :
	child(0) {
	}

	~Process() {
		if (child) {
			popen_plus_kill(child);
			popen_plus_close(child);
		}
	}

	void start(const std::string& cmdline) {
		child = popen_plus(cmdline.c_str());
		if (child == 0) {
			std::string msg = "Process could not start!\nCmdline: ";
			msg += cmdline.c_str();
			msg += '\n';
			msg = formatLastError(msg);
			throw std::runtime_error(msg);
		}
	}

	void read(char *buffer, size_t count) {
		if (fread(buffer, 1, count, child->read_fp) != count) {
			std::string msg = formatLastError("Process.read(char*,size_t)");
			throw std::runtime_error(msg);
		}
	}

	void write(const char *buffer, size_t length) {
		if (fwrite(buffer, 1, length, child->write_fp) != length) {
			std::string msg = formatLastError("Process.write(char*,size_t)");
			throw std::runtime_error(msg);
		}
	}

	void flush() {
		fflush(child->write_fp);
	}
};

#endif

#endif
