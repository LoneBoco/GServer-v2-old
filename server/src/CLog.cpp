#include "IDebug.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "CLog.h"

static CString getBasePath();

CLog::CLog()
: enabled(false), file(0)
{
	m_write = new std::recursive_mutex();

	// Get base path.
	homepath = getBasePath();
}

CLog::CLog(const CString& _file, bool _enabled)
: enabled(_enabled), filename(_file), file(0)
{
	m_write = new std::recursive_mutex();

	// Get base path.
	homepath = getBasePath();

	// Open the file now.
	if (enabled)
		file = fopen((homepath + filename).text(), "a");
	if (0 == file)
		enabled = false;
}

CLog::~CLog()
{
	{
		std::lock_guard<std::recursive_mutex> lock(*m_write);

		if (file)
		{
			fflush(file);
			fclose(file);
		}
	}

	delete m_write;
}

void CLog::out(const CString format, ...)
{
	va_list s_format_v;

	std::lock_guard<std::recursive_mutex> lock(*m_write);

	// Assemble and print the timestamp.
	char timestr[60];
	time_t curtime = time(0);
	strftime(timestr, sizeof(timestr), "%I:%M %p", localtime(&curtime));
	printf("[%s] ", timestr);

	// Log output to file.
	if (true == enabled && 0 != file)
	{
		// Save the timestamp to the file.
		strftime(timestr, sizeof(timestr), "%a %b %d %X %Y", localtime(&curtime));
		fprintf(file, "[%s] ", timestr);

		// Write the message to the file.
		va_start(s_format_v, format);
		vfprintf(file, format.text(), s_format_v);
		va_end(s_format_v);
		fflush(file);
	}

	// Display message.
	va_start(s_format_v, format);
	vprintf(format.text(), s_format_v);
	va_end(s_format_v);
}

void CLog::append(const CString format, ...)
{
	va_list s_format_v;
	va_start(s_format_v, format);

	std::lock_guard<std::recursive_mutex> lock(*m_write);

	// Log output to file.
	if (true == enabled && 0 != file)
	{
		// Write the message to the file.
		vfprintf(file, format.text(), s_format_v);
		fflush(file);
		va_end(s_format_v);
		va_start(s_format_v, format);
	}

	// Display message.
	vprintf(format.text(), s_format_v);

	va_end(s_format_v);
}

void CLog::clear()
{
	std::lock_guard<std::recursive_mutex> lock(*m_write);

	if (file) fclose(file);

	file = fopen((homepath + filename).text(), "w");
	if (0 == file) enabled = false;
	else enabled = true;
}

void CLog::close()
{
	std::lock_guard<std::recursive_mutex> lock(*m_write);

	if (file) fclose(file);
	file = 0;
	enabled = false;
}



void CLog::open()
{
	std::lock_guard<std::recursive_mutex> lock(*m_write);

	if (file) fclose(file);

	file = fopen((homepath + filename).text(), "a");
	if (0 == file) enabled = false;
	else enabled = true;
}

void CLog::setFilename(const CString& filename)
{
	std::lock_guard<std::recursive_mutex> lock(*m_write);

	if (file) fclose(file);

	this->filename = filename;
	file = fopen((homepath + filename).text(), "a");
	if (0 == file) enabled = false;
	else enabled = true;
}

CString getBasePath()
{
	CString homepath;

#if defined(_WIN32) || defined(_WIN64)
	// Get the path.
	char path[MAX_PATH];
	GetModuleFileNameA(0, path, MAX_PATH);

	// Find the program exe and remove it from the path.
	// Assign the path to homepath.
	homepath = path;
	int pos = homepath.findl('\\');
	if (pos == -1) homepath.clear();
	else if (pos != (homepath.length() - 1))
		homepath.removeI(++pos, homepath.length());
#else
	// Get the path to the program.
	char path[ 260 ];
	memset((void*)path, 0, 260);
	readlink("/proc/self/exe", path, sizeof(path));

	// Assign the path to homepath.
	char* end = strrchr(path, '/');
	if (end != 0)
	{
		end++;
		if (end != 0) *end = '\0';
		homepath = path;
	}
#endif

	return homepath;
}
