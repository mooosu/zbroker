#include "common.h"
using namespace zbroker;
using namespace boost;
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
     //stats
     m_query_doc_count = 0;
     m_query_count = 0;
     m_update_count = 0;
     m_queue.set_size(m_queue_size);
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

          m_limit = m_options.getIntField("limit");
          m_skip = m_options.getIntField("skip");
          m_queue_size = m_options.getIntField("queue_size");

          m_conditions = m_options.getObjectField("conditions");
          m_fields = m_options.getObjectField("fields");

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
          m_docset = m_database + "." + m_collection;
          m_queue.set_size(m_queue_size );
          m_inited = true;
     }
}
void broker::open(BSONObj* options )
{
     init(options);
     size_t buf_size = m_host.size() + 10;
     char* buf =  new char[buf_size];
     string errmsg ;

     sprintf(buf,"%s:%d",m_host.c_str(),m_port);
     m_connection_string = buf;
     delete buf;
     if(!m_connection.connect(m_connection_string.c_str(),errmsg)){
          throw broker_exception(errmsg.c_str());
     }
     m_connected = true;
}
void broker::check_status()
{
     if( !m_inited ){
          throw broker_not_inited("broker not inited");
     }
     if( !m_connected ){
          throw broker_not_connected("broker not connected");
     }
}
vector<string>& broker::query(mongo_sort sort)
{
     BSONObj doc;
     BSONObjBuilder builder;
     BSONObj new_conditions;
     int queryOptions = 0 ;
     int batchSize = 0;

     boost::mutex::scoped_lock lock(m_rewind_mutex);

     if( !m_last_doc_id.empty() ){
          OID id;
          id.init(m_last_doc_id);
          new_conditions = builder.appendElements(m_conditions).append("_id",BSONObjBuilder().append("$gt",id).obj()).obj();
     } else {
          new_conditions = m_conditions ;
     }
     Query query= Query(new_conditions);
     query.sort("_id",(int)sort);
     auto_ptr<DBClientCursor> cursor = m_connection.query(m_docset, 
               query,m_limit,m_skip,&m_fields,queryOptions,batchSize);
     bool has_docs = cursor->more();
     m_json_doc_cache.clear();
     m_last_doc_id.clear();
     while( cursor->more() ) {
          doc = cursor->next();
          m_json_doc_cache.push_back(doc.jsonString());
     }
     BSONElement e;
     if( has_docs ){
          BOOST_ASSERT( doc.getObjectID(e));
          m_last_doc_id = e.OID().str();
     }
     m_query_count ++;
     return m_json_doc_cache ;
}

void broker::read(size_t seconds)
{
     while( !m_do_exit ){
          vector<string> docs = query();
          if( docs.size() == 0 ){
               m_reach_end = true;
               cout << "no more items, sleep(5)" << endl;
               sleep(5);
               continue;
          }
          for(size_t i = 0 ; !m_do_exit && i< docs.size() ; i++ ){
               try{
                    m_queue.push(docs[i],seconds);
               }catch( broker_timeout ex ){
                    if( m_do_exit ){
                         cout << "get exiting" << endl;
                         break;
                    } else{
                         i--;
                    }
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
     while(!m_do_exit){
          string json =m_queue.pop();
          if(json.size() == 4 && json.find("exit")!=string::npos){
               cout << "get exit signal" << endl;
               break;
          }
          BSONObj obj =fromjson(json);
          BSONObj query = obj.getObjectField("query");
          BSONObj doc = obj.getObjectField("doc");
          bool upsert = obj.getBoolField("upsert");
          bool multi = obj.getBoolField("multi");
          m_connection.update(m_docset,query,doc,upsert,multi);
          m_update_count ++;
     }
}
