#ifndef _BROKER_HPP_
#define _BROKER_HPP_
#include "sized_queue.hpp"
#include <boost/thread/condition.hpp>
namespace zbroker{
     enum mongo_sort{
          Asc = 1,
          Desc = -1
     };
     class broker: boost::noncopyable{
          private:
               boost::mutex m_rewind_mutex;
               boost::mutex m_update_done_mutex;
               boost::mutex m_query_done_mutex;

               boost::condition m_con_rewind;
               boost::condition m_con_can_exit;
               boost::condition m_con_query;

               // docs
               zbroker::sized_queue<string> m_queue;
               vector<string> m_json_doc_cache;

               // mongo
               string m_connection_string;
               DBClientConnection m_connection;
               string m_docset ; 
               string m_last_doc_id;

               //config
               BSONObj m_options;

               string  m_host;
               int     m_port;

               string  m_database;
               string  m_collection;
               int     m_limit;
               int     m_skip;
               int     m_queue_size;
               int     m_read_timeout;
               int     m_read_retry;
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
               bool    m_querying;
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
               BSONObj prepare_condition();

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
               void close(){ reset(); }
               void read(size_t seconds = 0);
               void update();
               void rewind();

               void set_exit(){ m_do_exit = true; }
               void wait_update_done();

               size_t push(vector<string>& docs );
               string pop(size_t seconds = 0);

               size_t batch_pop( vector<string>&docs, size_t batch_size);
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
               static string hash(BSONObj &obj);

               string& get_last_doc_id();
     };
};
using zbroker::broker;
#endif
