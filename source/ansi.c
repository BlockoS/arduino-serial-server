#include "ansi.h"

/// Reset ANSI parser.
/// \param [in] parser ANSI parser structure.
void ansi_parser_reset(struct ansi_parser_t* parser)
{
	parser->input_state = ANSI_NORMAL;
	parser->escape_buffer_last     = 0;
	parser->ansi_string_buffer[0]  = 0;
	parser->ansi_string_buffer_len = 0;
	parser->ansi_string_received   = 0;
	parser->command_buffer_len     = 0;
}

/// Translate escape sequence into commands.
/// \note Only cursor controls are translated.
/// \param [in] parser Ansi parser structure.
/// \return
/// 	-1 if the command stack is full
///		 0 if the escape sequence was filtered out.
///		 1 if the escape sequence was succesfully translated.
int ansi_process_escape(struct ansi_parser_t* parser)
{
	int escape_buffer_last = parser->escape_buffer_last;
	uint8_t cmd = 0;
	
	// The command stack is full. We should wait until we have some
	// space left.
	if(parser->command_buffer_len >= MAX_COMMAND_STACK_SIZE)
	{
		return -1;
	}
	
	parser->escape_buffer_last = 0;
	
	// Filter escape sequences.
	// We are only interested in cursor controls. This means sequences of 
	// 3 or 4 characters and starting with ESC and [.
	if((escape_buffer_last < 2) || (escape_buffer_last > 3))
	{
		return 0;
	}
	if((parser->escape_buffer[0] != 0x1B) || (parser->escape_buffer[1] != 0x5B))
	{
		return 0;
	}
	
	// Translate escape sequence.
	if(escape_buffer_last == 2)
	{
		switch(parser->escape_buffer[2])
		{
			case ESC_UP:
				cmd = CMD_UP;
				break;
			case ESC_DOWN:
				cmd = CMD_DOWN;
				break;
			case ESC_RIGHT:
				cmd = CMD_RIGHT;
				break;
			case ESC_LEFT:
				cmd = CMD_LEFT;
				break;
			default:
				return 0;
		}
	}
	else if((escape_buffer_last == 3) && (parser->escape_buffer[3] == 0x7E))
	{
		switch(parser->escape_buffer[2])
		{
			case ESC_PGUP:
				cmd = CMD_PGUP;
			break;
			case ESC_PGDOWN:
				cmd = CMD_PGDOWN;
			break;
			default:
				return 0;
		}
	}
	
	parser->command_buffer[parser->command_buffer_len++] = cmd;

	return 1;
}

/// Erase previously read character.
/// \param [in] paser ANSI parser structure.
static void ansi_erase_previous_char(struct ansi_parser_t* parser)
{
	if((parser->input_state == ANSI_CSI) && (parser->escape_buffer_last == 1))
	{
		parser->input_state = ANSI_ESCAPE;
		parser->escape_buffer_last = 0;
	}
	else if((parser->input_state == ANSI_ESCAPE_SEQUENCE) && (parser->escape_buffer_last == 1))
	{
		parser->input_state = ANSI_ESCAPE;
		parser->escape_buffer_last = 0;
	}
	else if((parser->input_state == ANSI_ESCAPE) && (parser->escape_buffer_last == 0))
	{
		parser->input_state = ANSI_NORMAL;
	}
	else if(parser->input_state == ANSI_TERMINATOR)
	{
		parser->input_state = ANSI_STRING;
	}
	else if(parser->input_state == ANSI_STRING)
	{
		if(parser->ansi_string_buffer_len > 0)
		{
			--parser->ansi_string_buffer_len;
		}
		else
		{
			parser->input_state = ANSI_ESCAPE;
		}
	}
	else if(parser->escape_buffer_last > 0)
	{
		--parser->escape_buffer_last;
	}
}

/// Decode current char.
/// This is a shameless rip of http://www.mpp.mpg.de/~huber/vmssig/src/C/RMESCSEQ.C
/// \param [in] parser ANSI parser structure.
/// \param [in] data Current character read from the serial stream.
/// \return
///		-2  An escape sequence is ready to be treated.
///		-1  A sequence is being read. 
///		>=0 Character ready to be processed.
char ansi_decode(struct ansi_parser_t* parser, unsigned char data)
{
	if(parser->input_state == ANSI_NORMAL)
	{
		if(data == CH_ESCAPE)
		{
			// A new escape sequence is starting.
			parser->input_state = ANSI_ESCAPE;
			parser->escape_buffer[0]  = data;
			parser->escape_buffer_last = 0;
		}
		else if(data == CH_CSI)
		{
			// The escape sequence was restarted.
			parser->input_state = ANSI_CSI;
			// Save sequence in case it has to be replayed.
			parser->escape_buffer[0]   = data;
			parser->escape_buffer[1]   = '[';
			parser->escape_buffer_last = 1;
		}
		else
		{
			// This is a "standard" input.
			return data;
		}
		return -1;
	}
	 
	// Escape sequence.
	if((data < CH_SPACE) || (data == CH_CSI))
	{
		switch(data)
		{
			case CH_CANCEL:
			case CH_SUBSTITUTE:
				// Cancel escape sequence and go back to normal.
				parser->input_state = ANSI_NORMAL;
				parser->ansi_string_received   = 0;
				parser->ansi_string_buffer[0]  = 0;
				parser->ansi_string_buffer_len = 0;
				break;
			 
			case CH_BACKSPACE:
				// Erase previous characters.
				ansi_erase_previous_char(parser);
				break;
			 
			case CH_ESCAPE:
				if(parser->input_state == ANSI_STRING)
				{
					parser->input_state = ANSI_TERMINATOR;
				}
				else
				{
					parser->input_state = ANSI_ESCAPE;
					parser->escape_buffer[0]   = CH_ESCAPE;
					parser->escape_buffer_last = 0;
				}
				break;
			 
			case CH_CSI:
				parser->input_state = ANSI_CSI;
				parser->escape_buffer[0] = CH_CSI;
				parser->escape_buffer[1] = '[';
				parser->escape_buffer_last = 1;
				break;
			 
			default:
				if(data != CH_NUL)
				{
					return data;
				}
			break;
		}
		return -1;
	}
	 
	// Put bytes into the escape sequence buffer
	if((parser->input_state != ANSI_STRING) && (parser->input_state != ANSI_TERMINATOR))
	{
		if(parser->escape_buffer_last < MAX_ESCAPE_BUFFER_SIZE)
		{
			parser->escape_buffer[++parser->escape_buffer_last] = data;
		}
	}
	
	switch(parser->input_state)
	{
		case ANSI_ESCAPE:
			switch(data)
			{
				case '[':
					parser->input_state = ANSI_CSI;
					break;
				case '_':
				case 'P':
				case 'Q':
				case 'R':
				case 'X':
				case '^':
				case ']':
					parser->input_state = ANSI_STRING;
					parser->ansi_string_received   = 1;
					parser->ansi_string_buffer_len = 0;
					break;
				default:
					if((data > 57) && (data < 177))
					{
						parser->input_state = ANSI_NORMAL;
						return -2;
					}
					else
					{
						parser->input_state = ANSI_ESCAPE_SEQUENCE;
					}
			};
			break;
		case ANSI_ESCAPE_SEQUENCE:
			if((data > 57) && (data < 177))
			{
				parser->input_state = ANSI_NORMAL;
				return -2;
			}
		case ANSI_CSI:
			if((data > 64) && (data < 177))
			{
				parser->input_state = ANSI_NORMAL;
				return -2;
			}
		case ANSI_STRING:
			if(data == CH_ESCAPE)
			{
				parser->input_state = ANSI_TERMINATOR;
			}
			else if(data == CH_ST)
			{
				parser->input_state = ANSI_NORMAL;
				parser->ansi_string_received = 0;
			}
			else if(parser->ansi_string_received)
			{
				if(parser->ansi_string_buffer_len < MAX_ANSI_BUFFER_SIZE)
				{
					parser->ansi_string_buffer[parser->ansi_string_buffer_len++] = data;
				}
				else
				{
					parser->ansi_string_received = 0;
					parser->ansi_string_buffer_len = 0;
					parser->input_state = ANSI_NORMAL;
				}
			}
			break;
		case ANSI_TERMINATOR:
			if(data == '\\')
			{
				parser->input_state = ANSI_NORMAL;
				parser->ansi_string_received = 0;
			}
			else
			{
				if(data >= CH_SPACE)
				{
					parser->input_state = ANSI_STRING;
				}
				if(parser->ansi_string_buffer && ((parser->ansi_string_buffer_len+1) < MAX_ANSI_BUFFER_SIZE))
				{
					parser->ansi_string_buffer[parser->ansi_string_buffer_len++] = CH_ESCAPE;
					parser->ansi_string_buffer[parser->ansi_string_buffer_len++] = data;
				}
			}
			break;
		default:
			// nothig to do.
			break;
	}
	return -1;
}