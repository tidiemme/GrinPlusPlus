#include <Core/File.h>

#include <Common/Util/FileUtil.h>
#include <fstream>
#include <Windows.h>

static bool TruncateFile(const std::string& filePath, const uint64_t size)
{
	HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	LARGE_INTEGER li;
	li.QuadPart = size;
	const bool success = SetFilePointerEx(hFile, li, NULL, FILE_BEGIN) && SetEndOfFile(hFile);

	CloseHandle(hFile);

	return success;
}

File::File(const std::string& path)
	: m_path(path),
	m_bufferIndex(0),
	m_fileSize(0)
{

}

bool File::Load()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	std::ifstream file(m_path, std::ios::in | std::ifstream::ate | std::ifstream::binary);
	if (!file.is_open())
	{
		return false;
	}

	m_fileSize = file.tellg();
	m_bufferIndex = m_fileSize;
	file.close();

	if (m_fileSize > 0)
	{
		std::error_code error;
		m_mmap = mio::make_mmap_source(m_path, error);
	}

	return true;
}

bool File::Flush()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return Flush_Locked();
}

bool File::Flush_Locked()
{
	if (m_fileSize == m_bufferIndex && m_buffer.empty())
	{
		return true;
	}

	if (m_fileSize < m_bufferIndex)
	{
		return false;
	}

	m_mmap.unmap();

	if (m_fileSize > m_bufferIndex)
	{
		TruncateFile(m_path, m_bufferIndex);
	}

	if (!m_buffer.empty())
	{
		std::ofstream file(m_path, std::ios::out | std::ios::binary | std::ios::app);
		if (!file.is_open())
		{
			return false;
		}

		file.seekp(m_bufferIndex, std::ios::beg);

		if (!m_buffer.empty())
		{
			file.write((const char*)&m_buffer[0], m_buffer.size());
		}

		file.close();
	}

	m_fileSize = m_bufferIndex + m_buffer.size();
	
	m_bufferIndex = m_fileSize;
	m_buffer.clear();

	if (m_fileSize > 0)
	{
		std::error_code error;
		m_mmap = mio::make_mmap_source(m_path, error);
		if (error.value() > 0)
		{
			printf("ERROR");
		}
	}

	return true;
}

void File::Append(const std::vector<unsigned char>& data)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_buffer.insert(m_buffer.end(), data.cbegin(), data.cend());
}

bool File::Rewind(const uint64_t nextPosition)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!Flush_Locked())
	{
		return false;
	}

	if (nextPosition > m_fileSize)
	{
		return false;
	}

	m_bufferIndex = nextPosition;
	if (nextPosition <= m_bufferIndex)
	{
		m_buffer.clear();
		m_bufferIndex = nextPosition;
	}
	else
	{
		m_buffer.erase(m_buffer.begin() + nextPosition - m_bufferIndex, m_buffer.end());
	}

	return true;
}

bool File::Discard()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_bufferIndex = m_fileSize;
	m_buffer.clear();

	return true;
}

uint64_t File::GetSize() const
{
	std::lock_guard<std::mutex> lock(m_mutex);

	return m_bufferIndex + m_buffer.size();
}

bool File::Read(const uint64_t position, const uint64_t numBytes, std::vector<unsigned char>& data) const
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (position < m_bufferIndex)
	{
		data = std::vector<unsigned char>(m_mmap.cbegin() + position, m_mmap.cbegin() + position + numBytes);
	}
	else
	{
		const uint64_t firstBufferIndex = position - m_bufferIndex;

		data = std::vector<unsigned char>(m_buffer.cbegin() + firstBufferIndex, m_buffer.cbegin() + firstBufferIndex + numBytes);
	}

	return true;
}