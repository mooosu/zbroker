#include "common.h"
#include "asio_processor.hpp"

void asio_processor::term(){
     if( m_read_thread && m_read_broker.connected()){
          LOG(INFO) << red_text("***read thread terminiating***") << "processor id:" << color_id(m_processor_id) <<endl;
          m_read_broker.set_exit();
          if(m_read_broker.size() > 0 || !m_read_broker.reach_end()){
               vector<string> tmp;
               m_read_broker.batch_pop(tmp,1);
          } 
          pthread_join(m_read_thread,NULL);
          m_read_thread = NULL;
          m_read_broker.close();
          LOG(INFO) << blue_text("read thread terminiated") << ",processor id:" << color_id(m_processor_id) <<endl;
     }
     if( m_write_thread && m_write_broker.connected()){
          LOG(INFO) << red_text("***write thread terminiating***") << "processor id:" << blue_text(m_processor_id) <<endl;
          m_write_broker.set_exit();
          m_write_broker.wait_update_done();
          pthread_join(m_write_thread,NULL);
          m_write_thread = NULL;
          m_write_broker.close();
          LOG(INFO) << blue_text("write thread terminiated") << ",processor id:" << blue_text(m_processor_id) << endl;
     }
}
void asio_processor::open( Purpose p ,BSONObj& obj ){
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
     LOG(INFO) << red_text("\tPurpose:") << color_id(set_purpose_name(p)) << red_text(", refcount => ") << color_id(m_refcount) << endl;
}
void asio_processor::close(){
     if( m_refcount > 1 ){
          m_refcount--;
          LOG(INFO) << red_text("\tPurpose:") << color_id(get_purpose_name())<< red_text(", m_refcount:") << color_id(m_refcount) <<  endl;
     } else {
          LOG(INFO) << red_text("\tPurpose:") << color_id(get_purpose_name()) << red_text(", m_refcount: 0  @term") << endl;
          m_refcount = 0;
          term();
     }
}

void asio_processor::send_error( Response error ){
     if(error < ErrorMax && error> MinError ){
          int err_no = error - MinError;
          error_message* msg = &error_messages[err_no];
     }
}
string asio_processor::do_read(out_packet_ptr&packet){
     vector<string> docs;
     string ret;

     if( ULONG_MAX != m_read_broker.batch_pop(docs,m_read_broker.get_queue_size()) || docs.size() > 0){
          ret = pack_response(*packet.get(),OK,docs,"do_read");
     } else {
          ret = pack_response(*packet.get(),NoMoreItem,docs,"do_read no more items");
     }
     LOG(INFO) << "\t" << color_id(docs.size()) << " docs read" ;
     return ret;
}
bool asio_processor::do_rewind(){
     bool ret= false;
     if( m_read_broker.connected() ){
          m_read_broker.rewind();
          ret = true;
     }
     return ret;
}
string asio_processor::do_write(out_packet_ptr&packet,BSONObj& update){
     BSONObj obj = update.getObjectField("docs");
     vector<BSONObj> docs ;
     obj.Vals(docs);
     for(int i=0; i< docs.size();i++){
          m_write_broker.push(docs[i].jsonString());
     }
     LOG(INFO) << "\t" << color_id(docs.size()) << " docs written" ;
     return pack_response(*packet.get(),OK,"do_write");
}

string asio_processor::process( Response res, Command cmd , BSONObj& obj ){
     string ret ;
     out_packet_ptr packet(new out_packet());
     LOG(INFO) << "Entering @Processor:#" << color_id(m_processor_id) << ",Comand:" << color_id(get_command_name(cmd));

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
               case REWIND:
                    do_rewind();
                    ret = pack_response(*packet.get(),OK,"Rewind Action");
                    break;
               default:
                    throw "UnknownCommand";
                    send_error( UnknownCommand );
          }
     } else {
          throw "process error";
     }
     LOG(INFO) << "Leaving @Processor:#" << color_id(m_processor_id) << ",Comand:" << color_id(get_command_name(cmd));
     return ret;
}
string asio_processor::process( string& json)
{
     BSONObj obj;
     Command cmd;
     Response res = parse_request(obj,cmd,json);
     return process( res,cmd,obj);
}

///////////////////////////////////////////////////////////////
// Packet helpers
//////////////////////////////////////////////////////////////

Response asio_processor::parse_request(BSONObj &bodyObj,Command &cmd,const char* json)
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
Response asio_processor::parse_request(BSONObj &bodyObj,Command &cmd,string& json)
{
     return parse_request(bodyObj,cmd,json.c_str());
}
void asio_processor::init_response_builder(BSONObjBuilder&builder,Response res,BSONObj* dataToReturn,const char* extra)
{
     builder.append("response",(int)res);
     if(extra && extra[0] !=0 ){
          builder.append("extra",extra);
     }
     if( dataToReturn && !dataToReturn->isEmpty())
          builder.appendElements(*dataToReturn);
}
string asio_processor::get_response_string(out_packet& packet ,BSONObjBuilder& builder)
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
string asio_processor::pack_response(out_packet& packet,Response res,const char* extra ,BSONObj* dataToReturn)
{
     BSONObjBuilder builder;
     init_response_builder(builder,res,dataToReturn,extra);
     return get_response_string(packet,builder);
}
string asio_processor::pack_response(out_packet& packet ,Response res , vector<string>& docs,const char* extra)
{
     BSONObjBuilder docs_builder;
     init_response_builder(docs_builder,res,NULL,extra);
     if( docs.size() > 0 && res == OK ){
          docs_builder.append("docs",toBSONObj(docs));
     }
     return get_response_string(packet,docs_builder);

}

/*-------------------------------------------------------------*/

/*-------------------------------------------------------------*/
BSONObj asio_processor::toBSONObj(vector<string>& strs )
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

string& asio_processor::set_purpose_name( Purpose p ){
     if( m_purpose_name.empty() ){
          switch(p){
               case Read:
                    m_purpose_name = "Read";
                    break;
               case Write:
                    m_purpose_name = "Write";
                    break;
               default:
                    m_purpose_name = "Unkown";
                    break;
          }
     }
     return m_purpose_name;
}
const char* asio_processor::get_command_name( Command cmd)
{
     const char* ret =NULL;
     switch( cmd ){
          case OPEN:
               ret = "Open";
               break;
          case CLOSE:
               ret = "Close";
               break;
          case READ:
               ret = "Read";
               break;
          case WRITE:
               ret = "Write";
               break;
          case REWIND:
               ret = "Rewind";
               break;
          default:
               ret = "Unkown";
     }
     return ret;
}
