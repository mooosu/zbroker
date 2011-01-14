#ifndef COLOR_H
#define COLOR_H

/*  2 + (2 * num_attrs) + 8 + 1 + 8 + 'm' + NUL */
/* "\033[1;2;4;5;7;38;5;2xx;48;5;2xxm\0" */
/*
 * The maximum length of ANSI color sequence we would generate:
 * - leading ESC '['            2
 * - attr + ';'                 2 * 8 (e.g. "1;")
 * - fg color + ';'             9 (e.g. "38;5;2xx;")
 * - fg color + ';'             9 (e.g. "48;5;2xx;")
 * - terminating 'm' NUL        2
 *
 * The above overcounts attr (we only use 5 not 8) and one semicolon
 * but it is close enough.
 */
#define COLOR_MAXLEN 40

/*
 * IMPORTANT: Due to the way these color codes are emulated on Windows,
 * write them only using printf(), fprintf(), and fputs(). In particular,
 * do not use puts() or write().
 */
#define COLOR_NORMAL	""
#define COLOR_RESET		"\033[m"
#define COLOR_BOLD		"\033[1m"
#define COLOR_RED		"\033[31m"
#define COLOR_GREEN		"\033[32m"
#define COLOR_YELLOW	"\033[33m"
#define COLOR_BLUE		"\033[34m"
#define COLOR_MAGENTA	"\033[35m"
#define COLOR_CYAN		"\033[36m"
#define COLOR_BOLD_RED	"\033[1;31m"
#define COLOR_BOLD_GREEN	"\033[1;32m"
#define COLOR_BOLD_YELLOW	"\033[1;33m"
#define COLOR_BOLD_BLUE	"\033[1;34m"
#define COLOR_BOLD_MAGENTA	"\033[1;35m"
#define COLOR_BOLD_CYAN	"\033[1;36m"
#define COLOR_BG_RED	"\033[41m"
#define COLOR_BG_GREEN	"\033[42m"
#define COLOR_BG_YELLOW	"\033[43m"
#define COLOR_BG_BLUE	"\033[44m"
#define COLOR_BG_MAGENTA	"\033[45m"
#define COLOR_BG_CYAN	"\033[46m"

#include <string>
#include <cstdio>
using std::string;
using std::sprintf;
namespace zxlib{
     string color_text( const char* text , const char* color , bool bold = false );
     string red_text( const char* text,bool bold = true );
     string red_text( const string& text,bool bold = true);
     string blue_text( const char* text,bool bold = true );
     string blue_text( const string& text,bool bold = true);

     //
     //
     string color_begin( const char* color , bool bold );
     const char* color_end();
     string red_begin( bool bold = true );
     string blue_begin( bool bold = true );

     string color_id( const string& id_str );
     string color_id( size_t id );
};

#endif /* COLOR_H */
