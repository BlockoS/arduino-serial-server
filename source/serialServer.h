#ifndef SERIAL_SERVER_H
#define SERIAL_SERVER_H

#include "ansi.h"

/// User callback for serial events.
class SerialCallback
{
	public:
		virtual ~SerialCallback();
		
		/// This method will be called when a character is read.
		/// \param [in] data Alphanumeric or a space (' ', '\t', '\n', '\r').
		virtual void OnInput(char data) = 0;
		/// This method will be called for every command found in the stream.
		/// \param [in] command \see COMMAND.
		virtual void OnCommand(uint8_t command) = 0;
		/// This method will be called in the main loop after the serial stream
		/// and commands have been processed.
		virtual void Run() = 0;
};

/// Serial server.
class SerialServer
{
	public:
		SerialServer();
		~SerialServer();
		
		void Open(SerialCallback* callback);
		void Close();
		
		void Process(unsigned char data);
		void Run();
		
	protected:
		struct ansi_parser_t m_parser; ///< ANSI parser.
		SerialCallback* m_callback;    ///< User defined callback.
};

extern void serialEvent();

#endif // SERIAL_SERVER_H