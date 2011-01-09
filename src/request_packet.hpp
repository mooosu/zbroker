#ifndef REQUEST_PACKET_HPP
#define REQUEST_PACKET_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std; // For strncat and atoi.

class packet_header{
     public:
          enum { packet_id_length = 4 };
          enum { header_length = 16 };
          enum { max_body_length = 1024*1024};
     private:
          char m_header[header_length+1];
          size_t m_body_length;
     public:
          packet_header(){
               m_header[header_length]=0;
               m_body_length = 0;
          }
          char* buffer(){
               return m_header;
          }
          size_t length(){
               return header_length;
          }
          size_t decode(){
               m_body_length =  atoi(m_header);
               return m_body_length;
          }
          size_t body_length()
          {
               return m_body_length;
          }
};

class in_packet
{
     private:
          packet_header m_header;
          string        m_packet_id;
          char*         m_raw;
     public:
          packet_header& get_header(){
               return m_header;
          }
          size_t length(){
               return m_header.body_length()+m_header.length();
          }
          char* raw(){
               return m_raw;
          }
          char* body(){
               return m_raw+packet_header::packet_id_length;
          }
          string& get_packet_id(){
               return m_packet_id;
          }
          char* reserve(size_t size)
          {
               if( m_raw != NULL ){
                    delete m_raw;
                    m_raw = NULL;
               }
               m_raw = new char[size+1];
               m_raw[size] = 0;
               return m_raw;
          }
          in_packet(const char* raw , size_t size ){
               m_raw = NULL;
               unpack(raw,false);
          }
          bool unpack(){
               m_packet_id.assign(m_raw,packet_header::packet_id_length);
          }
          in_packet(){
               m_raw= NULL;
          }
          ~in_packet(){
               if( m_raw != NULL ){
                    delete m_raw;
                    m_raw = NULL;
               }
          }
     protected:
          bool unpack(const char* raw,bool unpack_only){
               memcpy(m_header.buffer(),raw,m_header.length());
               m_header.decode();

               size_t body_len = m_header.body_length();
               m_packet_id.assign(raw+m_header.length(),packet_header::packet_id_length);
               if( !unpack_only ){
                    memcpy(reserve(body_len),raw+m_header.length(),body_len);
               }
          }
};
class out_packet{
     private:
          string m_packet_id;
          string m_body;
          string m_data;
          bool   m_packed;
     public:
          out_packet(){
               m_packed = false;
          }
          void set_packet_id( const char* packet_id )
          {
               m_packet_id = packet_id;
          }

          void set_packet_id( string& packet_id )
          {
               m_packet_id = packet_id;
          }
          void set_body(string& body)
          {
               m_body = body;
          }
          size_t length()
          {
               return m_data.size();
          }
          const char* data(){
               return m_data.data();
          }
          string& pack()
          {
               using namespace std; // For sprintf and memcpy.
               char header[packet_header::header_length + 1] = "";
               size_t total_size = m_body.size()+m_packet_id.size();
               sprintf(header, "%16ld",total_size );
               string tmp = header;
               m_data.clear();
               m_data += header;
               m_data += m_packet_id;
               m_data += m_body;
               return m_data;
          }

};
typedef auto_ptr<out_packet> out_packet_ptr;
/*




          bool unpack()
          {
               m_packet_id.assign(m_raw,packet_id_length);
               m_body.assign(m_raw+packet_id_length,total_size-packet_id_length);
               return true;
          }
     private:
          string m_body;
          string m_packet_id;
          string m_data;
          size_t m_length;
          char* m_raw;
};
typedef auto_ptr<request_packet> request_packet_ptr;
*/


#endif // REQUEST_PACKET_HPP
