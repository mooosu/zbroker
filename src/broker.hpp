#ifndef _BROKER_HPP_
#define _BROKER_HPP_
#include <boost/thread/condition.hpp>
namespace zbroker{
     using namespace std;
     using namespace mongo;
     enum mongo_sort{
          Asc = 1,
          Desc = -1
     };
     class broker: noncopyable{
          private:
               boost::mutex m_rewind_mutex;
               boost::mutex m_update_done_mutex;

               boost::condition m_con_can_exit;
               size_t m_size;
               zbroker::sized_queue<string> m_queue;

               string m_connection_string;
               DBClientConnection m_connection;

               string m_docset ; 
               string m_last_doc_id;
               vector<string> m_json_doc_cache;

               //config
               BSONObj m_options;

               string  m_host;
               int     m_port;

               string  m_database;
               string  m_collection;
               int     m_limit;
               int     m_skip;
               int     m_queue_size;
               //conditions
               BSONObj m_conditions;
               // fields
               BSONObj m_fields;

               // status 
               bool    m_do_exit;
               bool    m_inited;
               bool    m_connected;
               bool    m_reach_end;
               bool    m_has_fields;
               // stats
               size_t m_query_doc_count;
               size_t m_query_count;
               size_t m_update_count;
          protected:
               void reset();
#ifndef BOOST_TEST_MODULE
               void init(BSONObj *options);
               vector<string>& query(mongo_sort sort=Asc);
#endif

               void check_status();

          public:
               // @options {
               //   :host=>"192.168.1.114",
               //   :port=> 27017,
               //   :database=> "zbot_development",
               //   :collection=> "analyse",
               //   :parameters={
               //   :skip=> 100,
               //   :limit => 1000,  # 0 for no limit
               //   :queue_size=>100,
               //   }
               //   :conditions => {} ,# {'brand'=>'Nokia'}
               //   :fields => ['brand',...]
               // }
               broker();
               ~broker();
               broker(BSONObj& options );

#ifdef BOOST_TEST_MODULE
               void init(BSONObj *options);
               vector<string>& query(mongo_sort sort=Asc);
#endif
               void open(BSONObj* options =NULL );
               void read(size_t seconds = 0);
               void update();
               void wait_update_done();


               void set_exit(){ m_do_exit = true; }
               void rewind() {
                    boost::mutex::scoped_lock lock(m_rewind_mutex);
                    m_last_doc_id.clear();
                    m_reach_end = false;
               }
               string pop(size_t seconds = 0){ 
                    check_status();
                    return m_queue.pop(seconds); 
               }
               size_t batch_pop( vector<string>&docs, size_t batch_size){
                    BOOST_ASSERT(batch_size > 0 );
                    try{
                         for( int i = 0 ; i< batch_size ; i++ ){
                              docs.push_back(pop(3));
                         }
                    } catch(broker_timeout &ex){
                         cout << "batch_pop timeout(3s):" << docs.size() << endl;
                         return ULONG_MAX;
                    }
                    return docs.size();
               }
               size_t push(string str  ){
                    check_status();
                    return m_queue.push(str); 
               }
               size_t size(){
                    check_status();
                    return m_queue.size(); 
               }
               DBClientConnection& get_connection(){ return m_connection;}
               string get_connection_string(){ return m_connection_string; }
               string get_docset(){ return m_docset; }
               string get_host(){ return m_host; }
               int    get_port(){ return m_port; }
               string get_database(){ return m_database; }
               string get_collection(){ return m_collection; }
               int    get_limit() { return m_limit; }
               int    get_skip() { return m_skip; }
               int    get_queue_size(){ return m_queue_size; }
               bool   inited() { return m_inited; }
               bool   connected() { return m_connected; }
               bool   reach_end() { return m_reach_end; }
               BSONObj get_fields() { return m_fields; }
               BSONObj get_conditions(){ return m_conditions; }
               size_t  get_query_doc_count(){ return m_query_doc_count;}
               size_t  get_query_count() { return m_query_count; }
               size_t  get_update_count() { return m_update_count; }
               string& get_last_doc_id() { return m_last_doc_id; }
               static string hash(BSONObj &obj);
     };
};
#endif
