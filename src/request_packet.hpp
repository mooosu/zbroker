#ifndef REQUEST_PACKET_HPP
#define REQUEST_PACKET_HPP

#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace std; // For strncat and atoi.

enum { request_id_length = 4 };
class packet_header{
     public:
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
          char*         m_raw;
     public:
          packet_header& get_header(){
               return m_header;
          }
          size_t length(){
               return m_header.body_length()+m_header.length();
          }
          char* body(){
               return m_raw;
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
               memcpy(m_header.buffer(),raw,m_header.length());

               size_t body_len = m_header.body_length();
               memcpy(reserve(body_len),raw+m_header.length(),body_len);
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
};
class out_packet{
     private:
          string m_request_id;
          string m_body;
          string m_data;
          bool   m_packed;
     public:
          out_packet(){
               m_packed = false;
          }
          void set_request_id( const char* request_id )
          {
               m_request_id = request_id;
          }

          void set_request_id( string& request_id )
          {
               m_request_id = request_id;
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
          string& pack(bool repack = false)
          {
               if( !m_packed || repack ){
                    using namespace std; // For sprintf and memcpy.
                    char header[packet_header::header_length + 1] = "";
                    size_t total_size = m_body.size()+m_request_id.size();
                    sprintf(header, "%16ld",total_size );
                    string tmp = header;
                    m_data += header;
                    m_data += m_request_id;
                    m_data += m_body;
                    m_packed = true;
               }
               return m_data;
          }

};
typedef auto_ptr<out_packet> out_packet_ptr;
/*




          bool unpack()
          {
               m_request_id.assign(m_raw,request_id_length);
               m_body.assign(m_raw+request_id_length,total_size-request_id_length);
               return true;
          }
     private:
          string m_body;
          string m_request_id;
          string m_data;
          size_t m_length;
          char* m_raw;
};
typedef auto_ptr<request_packet> request_packet_ptr;
*/


#endif // REQUEST_PACKET_HPP
