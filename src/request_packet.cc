#include "request_packet.hpp"

string out_packet::pack()
{
     char buffer[256];
     size_t total_size = m_body.size()+packet_header::packet_id_length;

     sprintf(buffer, "%16ld",total_size );
     string tmp = buffer;

     sprintf(buffer, "%32s",m_packet_id.c_str() );
     m_packet_id = buffer;

     m_data.clear();
     m_data += tmp;
     m_data += m_packet_id;
     m_data += m_body;
     return m_data;
}
void out_packet::set_body(string& body)
{
     m_body = body;
}
