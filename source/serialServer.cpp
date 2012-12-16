#include "serialServer.h"

/// Destructor.
SerialCallback::~SerialCallback()
{}

/// Constructor.
SerialServer::SerialServer()
	: m_callback(0)
{}
/// Destructor.
SerialServer::~SerialServer()
{}
		
/// Open serial and set user callback.
/// \param [in] callback User callback.
void SerialServer::Open(SerialCallback* callback)
{
	m_callback = callback;
	ansi_parser_reset(&m_parser);
}

/// Close serial.
void SerialServer::Close()
{}

/// Process data from serial input stream.
/// \param [in] data Byte from serial input.
void SerialServer::Process(unsigned char data)
{
	char ret;
	if((data > 127) && (data < 160) && (data != CH_CSI))
	{
		ret = ansi_decode(&m_parser, CH_ESCAPE);
		if(ret >= 0)
		{
			data = ret;
			if(m_callback)
			{
				m_callback->OnInput(data);
			}
		}
		else if(ret == -2)
		{
			ansi_process_escape(&m_parser);
		}
		data = (data & 0x7f) | 0x40;
	}
	ret = ansi_decode(&m_parser, data);
	if(ret >= 0)
	{
		if(m_callback)
		{
			m_callback->OnInput(data);
		}
	}
	else if(ret == -2)
	{
		ansi_process_escape(&m_parser);
	}
}

/// Serial stream has been processed. It's now time to evaluate command and let the callback
/// work.
void SerialServer::Run()
{
	if(m_callback)
	{
		for(uint8_t i=0; i<m_parser.command_buffer_len; ++i)
		{
			m_callback->OnCommand(m_parser.command_buffer[i]);
		}
	
		m_parser.command_buffer_len = 0;
	
		m_callback->Run();
	}
	else
	{
		m_parser.command_buffer_len = 0;
	}
}