#include "common.h"
namespace zbroker{
     using namespace std;
     using namespace mongo;
     class broker{
          private:
               size_t m_size;
               zbroker::sized_queue<string> m_queue;

               string m_connection_string;
               DBClientConnection m_connection;

               string m_docset ; 
               vector<string> m_json_doc_cache;

               //config
               BSONObj m_options;

               string  m_host;
               int     m_port;

               string  m_database;
               string  m_collection;
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
          protected:

               void init(BSONObj &options);
               vector<string>& query();
               void check_status();

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
               broker();
               broker(BSONObj options );
               void open(BSONObj& options );
               void read();
               void update();


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
               string get_connection_string(){ return m_connection_string; }
               string get_docset(){ return m_docset; }
               string get_host(){ return m_host; }
               int    get_port(){ return m_port; }
               string get_database(){ return m_database; }
               string get_collection(){ return m_collection; }
               int    get_limit() { return m_limit; }
               int    get_skip() { return m_skip; }
               bool   inited() { return m_inited; }
               bool   connected() { return m_connected; }
               BSONObj get_fields() { return m_fields; }
               BSONObj get_conditions(){ return m_conditions; }
     };
};
