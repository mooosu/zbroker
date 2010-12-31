#include "common.h"
namespace zbroker{
     using namespace std;
     using namespace mongo;
     class broker{
          private:
               size_t m_size;
               zbroker::sized_queue<string> m_queue;
               DBClientConnection m_connection;
               string m_connection_string;
               string m_docset ; 
               vector<string> m_json_doc_cache;

               //config
               BSONObj m_options;
               string  m_host;
               string  m_database;
               string  m_collection;
               int     m_port;
               int     m_limit;
               int     m_skip;
               //conditions
               BSONObj m_conditions;
               // fields
               BSONObj m_fields;

               // status 
               bool    m_do_exit;
               bool    m_inited;
               bool    m_connected;

          public:
               // @options {
               //   :host=>"192.168.1.114",
               //   :port=> 27017,
               //   :database=> "zbot_development",
               //   :collection=> "analyse",
               //   :skip=> 100,
               //   :limit => 1000,  # 0 for no limit
               //   :conditions => {} ,# {'brand'=>'Nokia'}
               //   :fields => ['brand',...]
               // }
               broker(BSONObj options ){
                    m_do_exit = true;
                    
                    m_inited = false;
                    m_connected = false;
 
                    m_options = options.copy();
               }
               broker(){
                    m_do_exit = true;

                    m_inited = false;
                    m_connected = false;
                    m_port = 0;
                    m_limit = 0;
                    m_skip = 0 ;
                    m_queue.set_size(0);
               }
               void init(BSONObj &options){
                    if(!m_inited){
                         m_options = options.copy();
                         m_host = m_options.getStringField("host");
                         m_port = m_options.getIntField("port");
                         m_database = m_options.getStringField("database");
                         m_collection = m_options.getStringField("collection");
                         
                         m_limit = m_options.getIntField("limit");
                         m_skip = m_options.getIntField("skip");

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
                              m_limit = 0 ;
                         }
                         if( m_skip  == INT_MIN ){
                              m_skip = 0 ;
                         }
                         m_docset = m_database + "." + m_collection;
                         m_inited = true;
                    }
               }
               void open(BSONObj& options )
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
               void check_status()
               {
                    if( !m_inited ){
                         throw broker_not_inited("broker not inited");
                    }
                    if( !m_connected ){
                         throw broker_not_connected("broker not connected");
                    }
               }


               void set_exit( ){ m_do_exit = true; }
               string pop(size_t seconds = 0){ 
                    check_status();
                    return m_queue.pop(seconds); 
               }
               size_t push(string str  ){
                    check_status();
                    return m_queue.push(str); 
               }
               size_t size(){ 
                    check_status();
                    return m_queue.get_size(); 
               }
               vector<string>& query()
               {
                    BSONObj doc;
                    int queryOptions = 0 ;
                    int batchSize = 0;
                    auto_ptr<DBClientCursor> cursor = m_connection.query(m_docset, 
                              Query(m_conditions),m_limit,m_skip,&m_fields,queryOptions,batchSize);
                    m_json_doc_cache.clear();
                    while( cursor->more() ) {
                         doc = cursor->next();
                         m_json_doc_cache.push_back(doc.jsonString());
                    }
                    return m_json_doc_cache ;
               }
               void read()
               {
                    while( !m_do_exit ){
                         vector<string> docs = query();
                         if( docs.size() == 0 ){
                              cout << "no more items, sleep(5)" << endl;
                              sleep(5);
                         }
                         for(size_t i = 0 ; i< docs.size() ; i++ ){
                              m_queue.push(docs[i]);
                         }
                    }
               }
               void update()
               {
                    while(!m_do_exit){
                         string json_query=m_queue.pop();
                         BSONObj query =fromjson(json_query);
                         m_connection.query(m_docset,query);
                    }
               }
     };
};
