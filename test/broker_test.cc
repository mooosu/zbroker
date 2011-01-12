#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE broker_test
#include <iostream>
#include "common.h"
#include "broker.hpp"
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace zbroker;
using namespace mongo;

struct broker_test
{
     string m_json_string;
     string m_json_string2;
     string m_json_string3;
     broker_test(){
          m_json_string ="{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                        \"collection\":\"broker\",\"parameters\":{\"skip\":100,\"limit\":1000, \"queue_size\":100}, \
                        \"conditions\":{\"brand\":\"Nokia\"},\"fields\":{\"category\":1,\"brand\":1}, \
                        \"upsert\":false,\"multi\":true,\"purpose\":1}";

          m_json_string2 ="{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                        \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":100, \"queue_size\":2}, \
                        \"conditions\":{\"brand\":\"Nokia\"}, \"fields\":{\"category\":1,\"brand\":1}, \
                        \"upsert\":false,\"multi\":true,\"purpose\":1}";

          m_json_string3 ="{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                        \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                        \"conditions\":{\"brand\":\"Nokia\"}, \"fields\":{\"category\":1,\"brand\":1}, \
                        \"upsert\":false,\"multi\":true,\"purpose\":1}";
     }
};

BOOST_FIXTURE_TEST_SUITE(mytest, broker_test);

// test fields
BOOST_AUTO_TEST_CASE(test_fields)
{
     BSONObj obj = fromjson(m_json_string);
     BSONObj parameters = obj.getObjectField("parameters");
     BOOST_CHECK_EQUAL(obj.getIntField("port"),27017);
     BOOST_CHECK_EQUAL(parameters.getIntField("skip"),100);
     BOOST_CHECK_EQUAL(parameters.getIntField("limit"),1000);
     BOOST_CHECK_EQUAL(parameters.getIntField("queue_size"),100);
     BOOST_CHECK_EQUAL(obj.getStringField("host"),"192.168.1.86");
     BOOST_CHECK_EQUAL(obj.getStringField("database"),"zbroker");
     BOOST_CHECK_EQUAL(obj.getStringField("collection"),"broker");
     BSONObj conditions = obj.getObjectField("conditions");
     BOOST_CHECK_EQUAL(conditions.getStringField("brand"),"Nokia");

     BSONObj fields = obj.getObjectField("fields");
     BOOST_CHECK_EQUAL(fields.getIntField("brand"),1);
     BOOST_CHECK_EQUAL(fields.getIntField("category"),1);
}
BOOST_AUTO_TEST_CASE(test_broker_members)
{
     BSONObj obj = fromjson(m_json_string);
     broker test_bk;
     test_bk.init(&obj);
     BOOST_CHECK_EQUAL(test_bk.get_host(),"192.168.1.86");
     BOOST_CHECK_EQUAL(test_bk.get_port(),27017);
     BOOST_CHECK_EQUAL(test_bk.get_database(),"zbroker");
     BOOST_CHECK_EQUAL(test_bk.get_collection(),"broker");
     BOOST_CHECK_EQUAL(test_bk.get_docset(),"zbroker.broker");
     BOOST_CHECK_EQUAL(test_bk.get_limit(),1000);
     BOOST_CHECK_EQUAL(test_bk.get_skip(),100);
     BOOST_CHECK_EQUAL(test_bk.get_queue_size(),100);
     BOOST_CHECK_EQUAL(test_bk.get_conditions(),obj.getObjectField("conditions"));
     BOOST_CHECK_EQUAL(test_bk.get_fields(),obj.getObjectField("fields"));
     BOOST_CHECK_EQUAL(test_bk.inited(),true);
     BOOST_CHECK_EQUAL(test_bk.connected(),false);
}
BOOST_AUTO_TEST_CASE(test_broker_open)
{
     BSONObj obj = fromjson(m_json_string);
     broker test_bk;
     test_bk.open(&obj);
     BOOST_CHECK_EQUAL(test_bk.inited(),true);
     BOOST_CHECK_EQUAL(test_bk.connected(),true);
     BOOST_CHECK_EQUAL(test_bk.get_connection_string(),"192.168.1.86:27017");
}
bool find_in_string(string& str1 , const char* str2) {
     return str1.find(str2)!=string::npos;
}

BOOST_AUTO_TEST_CASE(test_query)
{

     BSONObj obj = fromjson(m_json_string2);
     broker test_bk;
     test_bk.open(&obj);
     test_bk.get_connection().dropCollection("zbroker.broker");
     vector<string> strs = test_bk.query();
     BOOST_CHECK_EQUAL(strs.size() , 0 );
     const char* jsons[] = {
          "{\"brand\":\"麦图\",\"category\":\"SE\"}",
          "{\"brand\":\"Google\",\"category\":\"SE\"}",
          "{\"brand\":\"Microsoft\",\"category\":\"Software\"}",
          "{\"brand\":\"Nokia\",\"category\":\"Phone\"}",
          "{\"brand\":\"Nokia\",\"category\":\"OpenSource\"}"
     };
     for( int i =0 ; i< sizeof(jsons)/sizeof(char*);i++){
          test_bk.get_connection().insert(test_bk.get_docset(),fromjson(jsons[i]));
     }
     strs = test_bk.query();
     const char* matches[]={"OpenSource","Phone"};
     vector<const char*>vmatches(matches,matches+2);
     BOOST_CHECK_EQUAL(strs.size() , 2 );

     BOOST_CHECK(find_first_of(strs.begin(),strs.end(),vmatches.begin(),vmatches.end(),find_in_string) != strs.end());
     BOOST_CHECK(find_first_of(++strs.begin(),strs.end(),vmatches.begin(),vmatches.end(),find_in_string) != strs.end());
     //test sort
     vector<string> asc_ids ;
     vector<string> desc_ids ;
     for(int i=0;i< strs.size() ; i++ ){
          BSONObj tmp = fromjson(strs[i]);
          BSONElement e;
          tmp.getObjectID(e);
          asc_ids.push_back(e.OID().str());
     }
     test_bk.rewind();
     strs = test_bk.query(Desc);
     for(int i=0;i< strs.size() ; i++ ){
          BSONObj tmp = fromjson(strs[i]);
          BSONElement e;
          tmp.getObjectID(e);
          desc_ids.push_back(e.OID().str());
     }
     BOOST_CHECK_EQUAL(asc_ids.front(),desc_ids.back());
     BOOST_CHECK_EQUAL(asc_ids.back(),desc_ids.front());
     //test where
}

void *read_queue_thread(void *arg)
{
     broker *bk= (broker*)arg;
     bk->open();
     bk->read();
     return (NULL);
}
BOOST_AUTO_TEST_CASE(test_read)
{
     BSONObj obj = fromjson(m_json_string2);
     const int docs_count = 888;
     broker test_bk;
     test_bk.open(&obj);
     test_bk.get_connection().dropCollection("zbroker.broker");

     char buffer[1024];
     const char* json = "{\"brand\":\"Nokia\",\"category\":\"Category_%d\"}";
     for( int i =0 ; i< docs_count ;i++){
          sprintf(buffer,json,i);
          test_bk.get_connection().insert(test_bk.get_docset(),fromjson(buffer));
     }
     BOOST_CHECK_EQUAL( test_bk.query().size() , 100);

     broker bk_read(obj);

     pthread_t worker;
     pthread_create (&worker, NULL, read_queue_thread, (void *) &bk_read);
     int limit = 2;
     sleep(1);
     vector<string> docs ;
     bool found = false;
     size_t size = 0;
     while(true){
          if( bk_read.reach_end()) break;
          docs.push_back(bk_read.pop());
          if(docs.back().find("Category_887") != string::npos ){
               BOOST_CHECK_EQUAL(bk_read.size(),0);
               BOOST_CHECK(docs.back().find(bk_read.get_last_doc_id()) != string::npos );
               found = true;
               break;
          }
     }
     bk_read.set_exit();
     BOOST_CHECK(found);
     BOOST_CHECK_EQUAL(bk_read.get_query_count(),docs_count/bk_read.get_limit()+1);
     BOOST_CHECK_EQUAL(bk_read.get_queue_size(),2);
     sleep(6);
}
BOOST_AUTO_TEST_CASE(test_read_update)
{
     BSONObj obj = fromjson(m_json_string2);
     const int docs_count = 100;
     broker test_bk;
     test_bk.open(&obj);
     test_bk.get_connection().dropCollection("zbroker.broker");

     char buffer[1024];
     const char* json = "{\"brand\":\"Nokia\",\"category\":\"Category_%d\"}";
     for( int i =0 ; i< docs_count ;i++){
          sprintf(buffer,json,i);
          test_bk.get_connection().insert(test_bk.get_docset(),fromjson(buffer));
     }
     BOOST_CHECK_EQUAL( test_bk.query().size() , docs_count);
     BSONObj bq = BSONObjBuilder().append("category",BSONObjBuilder().append("$gt","Category_90").obj()).obj();
     BSONObj new_value = BSONObjBuilder().append("brand","newnew").obj();
     //update one doc
     test_bk.get_connection().update("zbroker.broker",bq,new_value,false,false);
     bq = BSONObjBuilder().append("category",BSONObjBuilder().append("$gt","Category_80").append("$lt","Category_90").obj()).obj();
     new_value = BSONObjBuilder().append("$set",BSONObjBuilder().append("brand","new").obj()).obj();
     test_bk.get_connection().update("zbroker.broker",bq,new_value,false,true);

     OID id ;
     id.init(test_bk.get_last_doc_id());

     BSONElement e;
     sprintf(buffer,"{ \"_id\" : { \"$oid\" : \"%s\" }}",test_bk.get_last_doc_id().c_str());
     BSONObj bo = fromjson(buffer);
     e = bo.getField("_id");
     cout << "type: "<< e.type() << endl;
     
     auto_ptr< DBClientCursor > cursor = test_bk.get_connection().query(test_bk.get_docset(),Query(BSONObjBuilder().append("_id",e.OID()).obj()));
     BOOST_CHECK( cursor->more());

}

void* update_queue_thread( void *arg )
{
     broker *bk= (broker*) arg;
     bk->open();
     bk->update();
     return (NULL);
}
BOOST_AUTO_TEST_CASE(test_update)
{
     BSONObj obj = fromjson(m_json_string2);
     const int docs_count = 100;
     broker test_bk;
     test_bk.open(&obj);
     test_bk.get_connection().dropCollection("zbroker.broker");

     char buffer[1024];
     const char* json = "{\"brand\":\"Nokia\",\"category\":\"Category_%d\"}";
     for( int i =0 ; i< docs_count ;i++){
          sprintf(buffer,json,i);
          test_bk.get_connection().insert(test_bk.get_docset(),fromjson(buffer));
     }
     vector<string> strs = test_bk.query();
     BOOST_CHECK_EQUAL( strs.size() , docs_count);

     vector<string> ids ;
     broker bk_update(obj);

     pthread_t worker;
     pthread_create (&worker, NULL, update_queue_thread, (void *) &bk_update);
     sleep(1);
     for(int i=0;i< strs.size() ; i++ ){
          BSONObj tmp = fromjson(strs[i]);
          BSONElement e;
          tmp.getObjectID(e);
          sprintf(buffer,"{\"query\":{\"_id\":{\"$oid\":\"%s\"}}, \
                    \"doc\":{\"brand\":\"updated\",\"status\":1234}, \
                    \"upsert\":false,\"multi\":false}"
                    ,e.OID().str().c_str());
          bk_update.push(buffer);
     }
     while(bk_update.size() != 0 );
     sleep(1);
     BOOST_CHECK_EQUAL(bk_update.get_update_count(),strs.size());
     bk_update.set_exit();
     bk_update.wait_update_done();
     sleep(2);
}
BOOST_AUTO_TEST_SUITE_END();

/*
 * vim:ts=5:sw=5:
 */
