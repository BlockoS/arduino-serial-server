#include <arduino.h>
#include "serialServer.h"
 
const char g_welcomeMsg[] = "Welcome!";

#define MAX_STRING_BUFFER_SIZE 256

/// Dummy serial callback.
class DummySerialCallback : public SerialCallback
{
	public:
		DummySerialCallback();
		~DummySerialCallback();
		
		void OnInput(char data);
		void OnCommand(uint8_t command);
		void Run();
		
	private:
		char m_buffer[MAX_STRING_BUFFER_SIZE];
		int  m_len;
		volatile bool m_ready;
};

/// Constructor.
DummySerialCallback::DummySerialCallback()
	: m_len(0)
	, m_ready(false)
{}

/// Destructor.
DummySerialCallback::~DummySerialCallback()
{}

/// Here we push characters into a buffer until we read '\n' or '\r',
/// and send back what was just read. This will emulate a dummy terminal.
void DummySerialCallback::OnInput(char data)
{
	if(data == 127)
	{
		if(m_len > 0)
		{
			Serial.print(data);
			m_buffer[--m_len] = '\0';
		}
	}
	else
	{
		if((data == '\r') || (data == '\n'))
		{
			data = '\0';
			Serial.println();
			m_ready = true;
		}
		else
		{
			Serial.print(data);
		}
		 
		if(m_len < MAX_STRING_BUFFER_SIZE)
		{
			m_buffer[m_len++] = data;
		}
	}
}

/// Process commands.
void DummySerialCallback::OnCommand(uint8_t command)
{
	switch(command)
	{
		case CMD_UP:
			Serial.println("Up!");
			break;
		case CMD_DOWN:
			Serial.println("Down!");
			break;
		case CMD_RIGHT:
			Serial.println("Right!");
			break;
		case CMD_LEFT:
			Serial.println("Left!");
			break;
		case CMD_PGUP:
			Serial.println("Page up!");
			break;
		case CMD_PGDOWN:
			Serial.println("Page down!");
			break;
	}
	Serial.print('>');
}

/// Output buffer if ready.
void DummySerialCallback::Run()
{
	if(!m_ready)
	{
		return;
	}
	
	int i, j;
	for(i=0, j=0; i<m_len; ++i)
	{
		if(m_buffer[i] == '\0')
		{
			if((i-j) >= 1)
			{
				Serial.print("\033[30m\033[47mString:\033[0m\033[0m");
				Serial.println(m_buffer+j);
			}
			j = i+1;
		}
	}
		 
	m_len = 0;
	Serial.print('>');
		 
	m_ready = false;
}

//--------------------------------------------------------
// Setup serial server and roll!
//--------------------------------------------------------
DummySerialCallback g_callback;
SerialServer g_serialServer;

/// Arduino serial event callback.
void serialEvent()
{
	while(Serial.available())
	{
		unsigned char data = Serial.read();
		g_serialServer.Process(data);
	}
}

/// Arduino setup.
void setup()
{
	g_serialServer.Open(&g_callback);
	
	Serial.begin(115200);
	Serial.println(g_welcomeMsg);
	Serial.print('>');
}

/// Arduino loop function.
void loop()
{
	g_serialServer.Run();
}
 
