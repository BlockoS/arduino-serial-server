#ifndef SERIAL_SERVER_ANSI_H
#define SERIAL_SERVER_ANSI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ESCAPE_BUFFER_SIZE 64
#define MAX_ANSI_BUFFER_SIZE 128
#define MAX_COMMAND_STACK_SIZE 32

/// "Standard" char.
enum CH
{
	CH_NUL        = 0x00,
	CH_BACKSPACE  = 0x08,
	CH_CANCEL     = 0x18,
	CH_SUBSTITUTE = 0x1a,
	CH_ESCAPE     = 0x1b,
	CH_SPACE      = 0x20,
	CH_CSI        = 0x9b,
	CH_ST         = 0x9c
};

/// Escape commands.
enum ESC
{
	ESC_PGUP   = 0x35,
	ESC_PGDOWN = 0x36,
	ESC_UP     = 0x41,
	ESC_DOWN   = 0x42,
	ESC_RIGHT  = 0x43,
	ESC_LEFT   = 0x44
};

/// ANSI parser state.
enum ANSI_INPUT_STATE
{
	ANSI_NORMAL = 0,
	ANSI_ESCAPE,
	ANSI_ESCAPE_SEQUENCE,
	ANSI_CSI,
	ANSI_STRING,
	ANSI_TERMINATOR
};

/// Commands.
/// \note Only cursor commands are handled atm.
enum COMMAND
{
	CMD_UP,
	CMD_DOWN,
	CMD_RIGHT,
	CMD_LEFT,
	CMD_PGUP,
	CMD_PGDOWN
};

/// ANSI parser.
struct ansi_parser_t
{
	/// Current parsing state.
	/// This tells if we are reading a alphanumeric char, an escape sequence, whatever...
	uint8_t input_state;

	/// Escape sequence buffer.
	/// This buffer will holds all the escape sequence read. This sequences will then
	/// be translated in a set of commands.
	char escape_buffer[MAX_ESCAPE_BUFFER_SIZE];
	/// Last byte of the escape sequence.
	int escape_buffer_last;

	/// ANSI string buffer.
	char ansi_string_buffer[MAX_ANSI_BUFFER_SIZE];
	/// Number of bytes used in the ansi string buffer.	
	int ansi_string_buffer_len;
	/// This flag tells if we are receiving a string.
	uint8_t ansi_string_received;
	
	/// Command buffer.
	/// Each time an escape sequence is read, it will be translated into a command.
	/// Note that not all escape sequence are translated. For the time being, only
	/// cursor constrols are used. The other are simply ignored.
	uint8_t command_buffer[MAX_COMMAND_STACK_SIZE];
	/// Number of bytes used in the command buffer.
	volatile uint8_t command_buffer_len;
};

void ansi_parser_reset(struct ansi_parser_t* parser);
int ansi_process_escape(struct ansi_parser_t* parser);
char ansi_decode(struct ansi_parser_t* parser, unsigned char data);

#ifdef __cplusplus
}
#endif

#endif // SERIAL_SERVER_ANSI_H