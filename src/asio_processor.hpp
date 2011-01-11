#ifndef _ASIO_PROCESSOR_H_
#define _ASIO_PROCESSOR_H_
#include "broker.hpp"

using mongo::BSONObj;
using mongo::fromjson;
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
     CLOSE = 103,
     MaxCmd = CLOSE
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
     AlreadyOpen    = 505,
     ErrorMax = AlreadyOpen+1,
     OK             = 200,
}Response;

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
          void send_error( Response error ){
               if(error < ErrorMax && error> MinError ){
                    int err_no = error - MinError;
                    error_message* msg = &error_messages[err_no];
               }
          }
          void term(){
               if( m_read_thread && m_read_broker.connected()){
                    LOG(INFO) << "asio_processor::term: read thread is terminiating ==> " << (void*)m_read_thread <<endl;
                    m_read_broker.set_exit();
                    if(m_read_broker.size() > 0 && !m_read_broker.reach_end()){
                         vector<string> tmp;
                         m_read_broker.batch_pop(tmp,1);
                    } 
                    pthread_join(m_read_thread,NULL);
                    m_read_thread = NULL;
                    m_read_broker.close();
               }
               if( m_write_thread && m_write_broker.connected()){
                    LOG(INFO) << "asio_processor::term: write thread is terminiating ==> " << (void*)m_read_thread <<endl;
                    m_write_broker.wait_update_done();
                    pthread_join(m_write_thread,NULL);
                    m_write_thread = NULL;
                    m_write_broker.close();
               }
          }
          void open( Purpose p ,BSONObj& obj ){
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
               m_refcount++;
               LOG(INFO) << "asio_processor::open m_refcount: " << m_refcount << endl;
          }
          void close(){
               if( m_refcount > 1 ){
                    m_refcount--;
               } else {
                    LOG(INFO)<< "asio_processor::close: do term" << endl;
                    m_refcount = 0;
                    term();
               }
               LOG(INFO) << "asio_processor::close m_refcount: " << m_refcount << endl;
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
                    packet->set_packet_id(m_processor_id);
                    switch( cmd ){
                         case OPEN:
                              open((Purpose)obj.getIntField("purpose"),obj);
                              ret = pack_response(*packet.get(),OK,"Open Action");
                              break;
                         case CLOSE:
                              close();
                              ret = pack_response(*packet.get(),OK,"Close Action");
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
          static void init_response_builder(BSONObjBuilder&builder,Response res,BSONObj* dataToReturn,const char* extra="")
          {
               builder.append("response",(int)res);
               if(extra && extra[0] !=0 ){
                    builder.append("extra",extra);
               }
               if( dataToReturn && !dataToReturn->isEmpty())
                    builder.appendElements(*dataToReturn);
          }
          static string& get_response_string(out_packet& packet ,BSONObjBuilder& builder)
          {
               string json =builder.obj().jsonString(); 
               if( json.size() < packet_header::max_body_length){
               } else {
                    json = BSONObjBuilder().append("response",(int)ResponseToLong).obj().jsonString();
                    LOG(WARNING) << "asio_processor::get_response_string: ResponseToLong" << endl;
               }
               packet.set_body(json);
               return packet.pack();
          }
          static string& pack_response(out_packet& packet,Response res,const char* extra="" ,BSONObj* dataToReturn=NULL)
          {
               BSONObjBuilder builder;
               init_response_builder(builder,res,dataToReturn,extra);
               return get_response_string(packet,builder);
          }

          static BSONObj toBSONObj(vector<string>& strs )
          {
               size_t i = 0;
               size_t len = strs.size();
               string tmp = "{ ";
               size_t buffer_len = 2048;
               char* buffer = new char[buffer_len];
               for( ; i< len-1; i++ ){
                    if( buffer_len < strs[i].size()+24 ){
                         delete buffer;
                         buffer_len +=24;
                         buffer = new char[buffer_len];
                    }
                    sprintf(buffer,"\"%ld\" : %s,",i,strs[i].c_str());
                    tmp+= buffer;
               }
               sprintf(buffer,"\"%ld\" : %s }",i,strs[i].c_str());
               tmp += buffer;
               delete buffer;
               return fromjson(tmp);
          }
          static string& pack_response(out_packet& packet ,Response res , vector<string>& docs,const char* extra="")
          {
               BSONObjBuilder docs_builder;
               init_response_builder(docs_builder,res,NULL,extra);
               if( docs.size() > 0 && res == OK ){
                    docs_builder.append("docs",toBSONObj(docs));
               }
               return get_response_string(packet,docs_builder);

          }
};
typedef asio_processor processor;
#endif
