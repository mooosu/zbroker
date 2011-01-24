#ifndef _ASIO_PROCESSOR_H_
#define _ASIO_PROCESSOR_H_
#include "protocol.h"
#include "broker.hpp"

namespace zbroker{

     class asio_processor:boost::noncopyable{
          private:
               broker m_read_broker;
               broker m_write_broker;
               char*  m_response_buf;
               size_t m_response_buf_size;
               pthread_t m_read_thread;
               pthread_t m_write_thread;
               string m_processor_id;
               size_t m_refcount;


               static void* write_thread( void *arg )
               {
                    LOG(INFO) << "write_thread: args == " << arg << endl;
                    broker *b= (broker*) arg;
                    b->update();
                    return (NULL);
               }
               static void* read_thread( void *arg )
               {
                    LOG(INFO) << "read_thread: args == " << arg << endl;
                    broker *b= (broker*) arg;
                    b->read();
                    return (NULL);
               }
          protected:
               void reset(string& processor_id ){
                    m_response_buf = NULL;
                    m_response_buf_size = 0;
                    m_read_thread = NULL;
                    m_write_thread = NULL;
                    BOOST_ASSERT(processor_id.size() > 0 );
                    m_processor_id = processor_id;
                    m_refcount = 0;
               }
               void wait_update_done() {
                    m_write_broker.wait_update_done();
               }
          public:
               asio_processor(string& processor_id){
                    reset(processor_id);
               }

               asio_processor(const char* processor_id){
                    string tmp = processor_id;
                    reset(tmp);
               }

               string& id(){return m_processor_id;}

               void open( Purpose p ,BSONObj& obj );


               void term();
               void close();

               void send_error( Response error );

               string do_read(out_packet_ptr&packet);
               string do_write(out_packet_ptr&packet,BSONObj& update);

               bool do_rewind();

               string process( Response res, Command cmd , BSONObj& obj );
               string process( string& json);


               static Response parse_request(BSONObj &bodyObj,Command &cmd,const char* json);
               static Response parse_request(BSONObj &bodyObj,Command &cmd,string& json);

               static void init_response_builder(BSONObjBuilder&builder,Response res,BSONObj* dataToReturn,const char* extra="");

               static string get_response_string(out_packet& packet ,BSONObjBuilder& builder);

               static string pack_response(out_packet& packet,Response res,const char* extra="" ,BSONObj* dataToReturn=NULL);
               static string pack_response(out_packet& packet ,Response res , vector<string>& docs,const char* extra="");

               static BSONObj toBSONObj(vector<string>& strs );

          // for debug log
          private:
               string m_purpose_name;
          public:
               string& set_purpose_name( Purpose p );
               string& get_purpose_name( ){ return m_purpose_name; }
               const char* get_command_name( Command cmd);

     };
     typedef asio_processor processor;
};
using zbroker::asio_processor;

#endif
