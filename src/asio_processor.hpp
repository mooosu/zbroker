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

class asio_processor:noncopyable{
     private:
          broker m_read_broker;
          broker m_write_broker;
          char*  m_response_buf;
          size_t m_response_buf_size;
          pthread_t m_read_thread;
          pthread_t m_write_thread;
          string m_processor_id;

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
     protected:
          void reset(string& processor_id ){
               m_response_buf = NULL;
               m_response_buf_size = 0;
               m_read_thread = NULL;
               m_write_thread = NULL;
               BOOST_ASSERT(processor_id.size() > 0 );
               m_processor_id = processor_id;
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
          void send_error( Response error ){
               if(error < ErrorMax && error> MinError ){
                    int err_no = error - MinError;
                    error_message* msg = &error_messages[err_no];
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
          static Response parse_request(BSONObj &bodyObj,Command &cmd,const char* json)
          {
               BSONObj cmdObj=fromjson(json);
               cmd = (Command)cmdObj.getIntField("cmd");
               if( cmd <= MaxCmd && cmd >= MinCmd ){
                    bodyObj = cmdObj.getObjectField("body").copy();
                    return OK;
               } else {
                    return UnknownCommand;
               }
          }
          static Response parse_request(BSONObj &bodyObj,Command &cmd,string& json)
          {
               return parse_request(bodyObj,cmd,json.c_str());
          }
          void init_response_builder(BSONObjBuilder&builder,Response res,BSONObj* dataToReturn,const char* extra="")
          {
               builder.append("response",(int)res);
               if(extra && extra[0] !=0 ){
                    builder.append("extra",extra);
               }
               if( dataToReturn && !dataToReturn->isEmpty())
                    builder.appendElements(*dataToReturn);
          }
          string& get_response_string(out_packet& packet ,BSONObjBuilder& builder)
          {
               string json =builder.obj().jsonString(); 
               if( json.size() > packet_header::max_body_length){
                    json = BSONObjBuilder().append("response",(int)ResponseToLong).obj().jsonString();
                    cout << "ResponseToLong" << endl;
               }
               packet.set_packet_id("xxxx");
               packet.set_body(json);
               return packet.pack();
          }
          string& pack_response(out_packet& packet,Response res,const char* extra="" ,BSONObj* dataToReturn=NULL)
          {
               BSONObjBuilder builder;
               init_response_builder(builder,res,dataToReturn,extra);
               return get_response_string(packet,builder);
          }
          string& pack_response(out_packet& packet ,Response res , vector<string>& docs,const char* extra="")
          {
               BSONObjBuilder docs_builder;
               init_response_builder(docs_builder,res,NULL,extra);
               if( docs.size() > 0 && res == OK ){
                    docs_builder.append<string>("docs",docs);
               }
               return get_response_string(packet,docs_builder);

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
          string& do_read(out_packet_ptr&packet){
               vector<string> docs;
               string ret;
               if( ULONG_MAX != m_read_broker.batch_pop(docs,m_read_broker.get_queue_size()) || docs.size() > 0){
                    return pack_response(*packet.get(),OK,docs,"do_read");
               } else {
                    return pack_response(*packet.get(),NoMoreItem,docs,"do_read no more items");
               }
          }
          string& do_write(out_packet_ptr&packet,BSONObj& update){
               BSONObj obj = update.getObjectField("docs");
               vector<BSONObj> docs ;
               obj.Vals(docs);
               for(int i=0; i< docs.size();i++){
                    m_write_broker.push(docs[i].jsonString());
               }
               return pack_response(*packet.get(),OK,"do_write");
          }
          string process( Response res, Command cmd , BSONObj& obj ){
               string ret ;
               out_packet_ptr packet(new out_packet());
               if( res == OK ){
                    switch( cmd ){
                         case OPEN:
                              setup_broker((Purpose)obj.getIntField("purpose"),obj);
                              ret = pack_response(*packet.get(),OK,"Open Action");
                              break;
                         case READ:
                              BOOST_ASSERT(m_read_broker.connected());
                              ret = do_read(packet);
                              break;
                         case WRITE:
                              BOOST_ASSERT(m_write_broker.connected());
                              ret = do_write(packet,obj);
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
          string process( string& json)
          {
               BSONObj obj;
               Command cmd;
               Response res = parse_request(obj,cmd,json);
               return process( res,cmd,obj);
          }
};
typedef asio_processor processor;
#endif
