#ifndef _ASIO_PROCESSOR_H_
#define _ASIO_PROCESSOR_H_
#include "broker.hpp"

using namespace mongo;
using namespace std;
using namespace zbroker;

typedef struct{
     const char* msg;
     size_t size;
}error_message;

#define MAX_REQUEST_MESSAGE_SIZE  (1024*1024)
#define ERROR_MESSAGE(x) {x,sizeof(x)-1}
static error_message error_messages[]={
     ERROR_MESSAGE( "Command not found"),
     ERROR_MESSAGE( "Command unimplemented yet!"),
     ERROR_MESSAGE( "Request body exceed limit(1MB)!"),
     ERROR_MESSAGE( "No more items!"),
     ERROR_MESSAGE( "Internal Error(invalid error code)")
};
typedef enum {
     ErrorCmd = -1,
     MinCmd = 100,
     OPEN  = 100,
     READ  = 101,
     WRITE = 102,
     MaxCmd = WRITE
}Command;
typedef enum{
     Read = 1,
     Write = 2
}Purpose;
typedef enum {
     MinError       = 500,
     UnknownCommand = 500,
     Unimplemented  = 501,
     RequestTooLong = 502,
     NoMoreItem     = 503,
     ResponseToLong = 504,
     ErrorMax = ResponseToLong+1,
     OK             = 200,
}Response;

class asio_processor{
     private:
          broker m_read_broker;
          broker m_write_broker;
          char*  m_response_buf;
          size_t m_response_buf_size;
          string m_raw_request;
          pthread_t m_read_thread;
          pthread_t m_write_thread;

          static void* write_thread( void *arg )
          {
               broker *b= (broker*) arg;
               b->update();
               return (NULL);
          }
          static void* read_thread( void *arg )
          {
               broker *b= (broker*) arg;
               b->read();
               return (NULL);
          }
     public:
          asio_processor(const char* request){
               m_response_buf = NULL;
               m_response_buf_size = 0;
               m_raw_request = request;
               m_read_thread = NULL;
               m_write_thread = NULL;
          }
          void send_error( Response error ){
               if(error < ErrorMax && error> MinError ){
                    int err_no = error - MinError;
                    error_message* msg = &error_messages[err_no];
                    request_packet reply ();
               }
          }
          void wait_update_done() {
               m_write_broker.wait_update_done();
          }
          void term(){
               if( m_read_thread && m_read_broker.connected()){
                    m_read_broker.set_exit();
                    if(m_read_broker.size() > 0 && !m_read_broker.reach_end()){
                         vector<string> tmp;
                         m_read_broker.batch_pop(tmp,1);
                    } 
                    pthread_join(m_read_thread,NULL);
               }
               if( m_write_thread && m_write_broker.connected()){
                    m_write_broker.set_exit();
                    m_write_broker.push("exit");
                    pthread_join(m_write_thread,NULL);
               }
          }
          Response parse_request(BSONObj &bodyObj,Command &cmd,const char* json)
          {
               if( m_raw_request.size() > MAX_REQUEST_MESSAGE_SIZE ){
                    return RequestTooLong;
               }

               BSONObj cmdObj=fromjson(json);
               cmd = (Command)cmdObj.getIntField("cmd");
               if( cmd <= MaxCmd && cmd >= MinCmd ){
                    bodyObj = cmdObj.getObjectField("body").copy();
                    return OK;
               } else {
                    return UnknownCommand;
               }
          }
          Response parse_request(BSONObj &bodyObj,Command &cmd)
          {
               return parse_request(bodyObj,cmd,m_raw_request.c_str());
          }
          size_t pack_response(request_packet& packet,Response res,const char* extra="" ){

               BSONObjBuilder docs_builder;
               docs_builder.append("response",(int)res);
               if(extra && extra[0] !=0 ){
                    docs_builder.append("extra",extra);
               }
               string json =docs_builder.obj().jsonString(); 

               memcpy(packet.body(),json.c_str(),json.size());
               packet.body_length(json.size());
               packet.encode_header();
               return packet.length();
          }
          size_t pack_response(request_packet& packet ,Response res , vector<string>& docs,const char* extra=""){
               BSONObjBuilder docs_builder;
               docs_builder.append("response",(int)res);
               if( docs.size() > 0 && res == OK ){
                    docs_builder.append<string>("docs",docs);
               }

               if(extra && extra[0] !=0 ){
                    docs_builder.append("extra",extra);
               }

               string json =docs_builder.obj().jsonString(); 
               if( json.size() > request_packet::max_body_length){
                    json = BSONObjBuilder().append("response",(int)ResponseToLong).obj().jsonString();
                    cout << "ResponseToLong" << endl;
               }

               memcpy(packet.body(),json.c_str(),json.size());
               packet.body_length(json.size());
               packet.encode_header();
               return packet.length();
          }
          void setup_broker( Purpose p ,BSONObj& obj ){
               switch(p){
                    case Read:
                         if( !m_read_broker.connected() ){
                              m_read_broker.open(&obj);
                              BOOST_ASSERT(0 == pthread_create (&m_read_thread, NULL,read_thread, (void *) &m_read_broker));
                              sleep(0);
                         }
                         break;
                    case Write:
                         if( !m_write_broker.connected() ){
                              m_write_broker.open(&obj);
                              BOOST_ASSERT(0 == pthread_create(&m_write_thread, NULL,write_thread, (void *) &m_write_broker));
                         }
                         break;
                    default:
                         throw "invalid Purpose";
                         break;
               }
          }
          void do_read(request_packet_ptr&packet){
               vector<string> docs;
               if( ULONG_MAX != m_read_broker.batch_pop(docs,m_read_broker.get_queue_size()) || docs.size() > 0){
                    pack_response(*packet.get(),OK,docs,"do_read");
               } else {
                    pack_response(*packet.get(),NoMoreItem,docs,"do_read no more items");
               }
          }
          void do_write(request_packet_ptr&packet,BSONObj& update){
               BSONObj obj = update.getObjectField("docs");
               vector<BSONObj> docs ;
               obj.Vals(docs);
               for(int i=0; i< docs.size();i++){
                    m_write_broker.push(docs[i].jsonString());
               }
               pack_response(*packet.get(),OK,"do_write");
          }
          string process( string& json  )
          {
               return process(json.c_str());
          }
          string process( const char* json  )
          {
               BSONObj obj;
               Command cmd;
               Response res = parse_request(obj,cmd,json);
               string ret ;
               request_packet_ptr packet(new request_packet());
               if( res == OK ){
                    switch( cmd ){
                         case OPEN:
                              setup_broker((Purpose)obj.getIntField("purpose"),obj);
                              pack_response(*packet.get(),OK,"Open Action");
                              ret = string(packet->data(),packet->length());
                              break;
                         case READ:
                              BOOST_ASSERT(m_read_broker.connected());
                              do_read(packet);
                              ret = string(packet->data(),packet->length());
                              break;
                         case WRITE:
                              BOOST_ASSERT(m_write_broker.connected());
                              do_write(packet,obj);
                              ret = string(packet->data(),packet->length());
                              break;
                         default:
                              throw "UnknownCommand";
                              send_error( UnknownCommand );
                    }
               } else {
                    throw "process error";
               }
               return ret;
          }
};
typedef asio_processor processor;
#endif
