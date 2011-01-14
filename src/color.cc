#include "color.h"
string zxlib::color_text( const char* text , const char* color , bool bold )
{
     string ret ;
     if( bold )
          ret += COLOR_BOLD;
     ret += color;
     ret += text;
     ret += COLOR_RESET;
     return ret;
}
string zxlib::red_text( const char* text , bool bold)
{
     return color_text(text,COLOR_RED,bold);
}
string zxlib::red_text( const string& text,bool bold)
{
     return red_text(text.c_str(),bold);
}

string zxlib::blue_text( const char* text , bool bold)
{
     return color_text(text,COLOR_BLUE,bold);
}
string zxlib::blue_text( const string& text,bool bold)
{
     return blue_text(text.c_str(),bold);
}

//
//
string zxlib::color_begin( const char* color , bool bold )
{
     string ret ;
     if( bold )
          ret += COLOR_BOLD;
     ret += color;
     return ret;
}
string zxlib::red_begin(bool bold)
{
     return color_begin(COLOR_RED,bold);
}
string zxlib::blue_begin(bool bold)
{
     return color_begin(COLOR_BLUE,bold);
}
const char* zxlib::color_end()
{
     return COLOR_RESET;
}
string zxlib::color_id( const string& id_str )
{
     string ret = "[" ;
     ret += blue_text(id_str);
     ret += "]";
     return ret;
}
string zxlib::color_id( size_t id)
{
     char buffer[128];
     std::sprintf(buffer,"%zd",id);
     string ret = "[" ;
     ret += blue_text(buffer);
     ret += "]";
     return ret;
}



#ifdef __TEST_COLOR__
#include <iostream>
using std::cout;
using std::endl;
using zxlib::red_text;
using zxlib::blue_text;
int main()
{
     cout << "[" << red_text("my red text") << "]" << endl;
     cout << "[" << red_text("my bold red text",true) << "]" << endl;
     cout << "[" << blue_text("my blue text") << "]" << endl;
     cout << "[" << blue_text("my bold blue text",true) << "]" << endl;
     return 0;
}

#endif
