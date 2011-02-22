#include "common.h"
#include "broker.hpp"
#include "boost/interprocess/exceptions.hpp"

broker::broker()
{
     reset();
}
broker::broker(BSONObj& options )
{
     reset();
     m_options = options.copy();
}
broker::~broker()
{
}
void broker::reset()
{
     m_do_exit = false;

     m_inited = false;
     m_connected = false;
     m_reach_end = false;
     m_port = 0;
     m_limit = 0;
     m_skip = 0 ;
     m_queue_size = 0;
     m_read_timeout = 3;
     m_read_retry = 3;
     //stats
     m_query_doc_count = 0;
     m_query_count = 0;
     m_update_count = 0;
     m_has_fields = false;

     boost::mutex::scoped_lock lock(m_rewind_mutex);
     m_last_doc_id.clear();
}
string broker::hash(BSONObj& obj){
     return obj.md5();
}

void broker::init(BSONObj *options)
{
     if(!m_inited){
          BOOST_ASSERT(!(m_options.isEmpty() && options == NULL ));
          if( options )
               m_options = options->copy();
          m_host = m_options.getStringField("host");
          m_port = m_options.getIntField("port");
          m_database = m_options.getStringField("database");
          m_collection = m_options.getStringField("collection");

          BSONObj parameters = m_options.getObjectField("parameters");

          m_limit = parameters.getIntField("limit");
          m_skip = parameters.getIntField("skip");
          m_queue_size = parameters.getIntField("queue_size");
          m_read_timeout =  parameters.getIntField("read_timeout");
 
          m_conditions = m_options.getObjectField("conditions");
          m_fields = m_options.getObjectField("fields");
          m_has_fields = m_fields.isEmpty() == false;

          if( m_host.empty() ){
               throw broker_argument_error("host must not be empty");
          }
          if( m_port == INT_MIN ){
               throw broker_argument_error("port must be set");
          }
          if( m_database.empty() ){
               throw broker_argument_error("database must not be empty");
          }
          if( m_collection.empty() ){
               throw broker_argument_error("collection must not be empty");
          }
          if( m_limit == INT_MIN ){
               m_limit = 1000 ;
          }
          if( m_skip  == INT_MIN ){
               m_skip = 0 ;
          }
          if( m_queue_size == INT_MIN ){
               m_queue_size = 100;
          }
          if( m_read_timeout == INT_MIN ){
               m_read_timeout = 3;
          } else if (m_read_timeout < 0 ) {
               m_read_timeout = 1;
          }
          m_docset = m_database + "." + m_collection;
          m_queue.clear();
          m_queue.set_size(m_queue_size );
          m_inited = true;

          LOG(INFO) << "broker::init: initialize completed!" << endl;
          LOG(INFO) << "\t host: " << m_host << endl;
          LOG(INFO) << "\t port: " << m_port << endl;
          LOG(INFO) << "\t database: " << m_database << endl;
          LOG(INFO) << "\t collection: " << m_collection << endl;
          LOG(INFO) << "\t conditions: " << m_conditions.jsonString() << endl;
          LOG(INFO) << "\t fields: " << m_fields.jsonString() << endl;
          LOG(INFO) << "\t limit: " << m_limit << endl;
          LOG(INFO) << "\t skip: " << m_skip<< endl;
          LOG(INFO) << "\t queue_size: " << m_queue_size << endl;
          LOG(INFO) << "\t read_timeout: " << m_read_timeout << endl;
          LOG(INFO) << "\t read_retry: " << m_read_retry << endl;
     }
}
void broker::open(BSONObj* options )
{
     init(options);
     size_t buf_size = m_host.size() + 10;
     auto_ptr<char> buf(new char[buf_size]);
     string errmsg ;

     sprintf(buf.get(),"%s:%d",m_host.c_str(),m_port);
     m_connection_string = buf.get();
     if(!m_connection.connect(m_connection_string.c_str(),errmsg)){
          throw broker_exception(errmsg.c_str());
     }
     m_connected = true;
}
void broker::check_status()
{
     if( !m_inited ){
          LOG(ERROR) << "broker::check_status: " << "broker not inited" << endl;
          throw broker_not_inited("broker not inited");
     }
     if( !m_connected ){
          throw broker_not_connected("broker not connected");
     }
}

size_t broker::push(vector<string>& docs )
{
     check_status();
     return m_queue.push(docs); 
}
string broker::pop(size_t seconds )
{ 
     check_status();
     return m_queue.pop(seconds); 
}

string& broker::get_last_doc_id() { 
     boost::mutex::scoped_lock lock(m_rewind_mutex);
     return m_last_doc_id; 
}
void broker::rewind() {
     boost::mutex::scoped_lock lock(m_rewind_mutex);
     m_last_doc_id.clear();
     m_reach_end = false;
}

BSONObj broker::prepare_condition()
{
     BSONObjBuilder builder;
     BSONObj new_conditions;
     if( !m_last_doc_id.empty() ){
          OID id;
          id.init(m_last_doc_id);
          new_conditions = builder.appendElements(m_conditions).append("_id",BSONObjBuilder().append("$gt",id).obj()).obj();
     } else {
          new_conditions = m_conditions ;
     }
     return new_conditions;
}
vector<string>& broker::query(mongo_sort sort)
{
     boost::mutex::scoped_lock lock(m_rewind_mutex);

     //query 
     BSONObj *fields=NULL;
     int queryOptions = 0 ;
     int batchSize = 0;

     Query query(prepare_condition());
     query.sort("_id",(int)sort);

     if( m_has_fields) fields = &m_fields;

     timer query_timer;
     auto_ptr<DBClientCursor> cursor; 
     cursor = m_connection.query(m_docset, query,m_limit,m_skip,fields,queryOptions,batchSize);
     double query_elapsed = query_timer.elapsed();

     // iterating
     timer iterating_timer;
     bool has_docs = cursor->more();
     m_json_doc_cache.clear();
     size_t count = 0;
     BSONObj doc;
     while( cursor->more() ) {
          doc = cursor->next();
          m_json_doc_cache.push_back(doc.jsonString());
          count++;
     }
     LOG(INFO) <<blue_text("query in ") << red_begin() << query_elapsed << "s" <<  color_end() << " & " 
         << red_begin() <<"iterating "  << count << " docs in " << iterating_timer.elapsed() <<  color_end() ;

     if( has_docs ){
          BSONElement e;
          BOOST_ASSERT( doc.getObjectID(e));
          m_last_doc_id = e.OID().str();
     }
     m_query_count ++;
     return m_json_doc_cache ;
}

size_t broker::batch_pop( vector<string>&docs, size_t batch_size){
     BOOST_ASSERT(batch_size > 0 );
     size_t ret = ULONG_MAX;
     int timeout = m_read_timeout;
     int retry= 0 ;
     do {
          try{
               docs.push_back(pop(timeout));
               for( int i = 0 ; i< batch_size-1 ; i++ ){
                    docs.push_back(pop(timeout));
               }
               ret = docs.size();
               break;
          } catch(broker_timeout &ex){
               retry++;
               LOG(WARNING) << "retry: " << retry << ",timeout: " << timeout << endl;
               timeout = m_read_timeout*retry;
          }
     }while(m_read_retry>= retry);
     return ret;
}
void broker::read(size_t seconds)
{
     size_t timeout_count = 0;
     while( !m_do_exit ){
          vector<string> docs = query();
          if( docs.size() == 0 ){
               m_reach_end = true;
               LOG_IF(INFO,timeout_count % 10 == 0) << "broker::read no more items, sleep(3),m_last_doc_id: "  << m_last_doc_id << endl;
               sleep(3);
               timeout_count++;
               continue;
          }
          m_reach_end = false;
          while( docs.size() > 0 && !m_do_exit ){
               try{
                    m_queue.push(docs,seconds);
               }catch( broker_timeout ex ){
                    if( m_do_exit ) break;
               }
          }
     }
}
/* op = {
 *   :query=>{"_id"=>ObjectID("doc_id")},
 *   :doc=>{"brand"=>new_value} || :doc=>{"$set"=>{"brand"=>new_value}},
 *   :upsert => false,:multi=>true
 * }
 *
 */
void broker::update()
{
     boost::mutex::scoped_lock lock(m_update_done_mutex);
     string json;
     size_t timeout_count = 0;
     while(true){
          try{
               json =m_queue.pop(3);
               BSONObj obj =fromjson(json);
               BSONObj query = obj.getObjectField("query");
               BSONObj doc = obj.getObjectField("doc");
               bool upsert = obj.getBoolField("upsert");
               bool multi = obj.getBoolField("multi");

               m_connection.update(m_docset,query,doc,upsert,multi);
               m_update_count ++;
          }catch (broker_timeout&ex){
               timeout_count ++ ;
               LOG_IF(INFO,timeout_count % 10 == 0) << "broker::update broker timeout(3s):" << ex.what() << endl;
               m_con_can_exit.notify_all();
               if(m_do_exit){
                    break;
               } else {
                    continue;
               }
          }
     }
}
void broker::wait_update_done()
{
     try{
          boost::mutex::scoped_lock lock(m_update_done_mutex);
     }catch(boost::interprocess::lock_exception e ){
          m_con_can_exit.wait(m_update_done_mutex);
     }
}
