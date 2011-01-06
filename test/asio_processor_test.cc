#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE asio_processor_test
#include <iostream>
#include "common.h"
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace zbroker;
using namespace mongo;

struct asio_processor_test
{
     string m_json_string;
     string m_json_string2;
     string m_json_open;
     string m_json_read;
     string m_json_read_no_data;
     string m_json_write;
     asio_processor_test(){
          m_json_string = "{\"cmd\":100, \
                           \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                           \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                           \"conditions\":{\"brand\":\"Nokia\"}, \"fields\":{\"category\":1,\"brand\":1}, \
                           \"upsert\":false,\"multi\":true,\"purpose\":1}}";
          m_json_open  = m_json_string;
          m_json_read = "{\"cmd\":101, \
                         \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                         \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                         \"conditions\":{\"brand\":\"Nokia\"}, \"fields\":{\"category\":1,\"brand\":1}, \
                         \"upsert\":false,\"multi\":true,\"purpose\":1}}";
          m_json_read_no_data = "{\"cmd\":101, \
                                 \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                                 \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                                 \"conditions\":{\"brand\":\"123123123\"}, \"fields\":{\"category\":1,\"brand\":1}, \
                                 \"upsert\":false,\"multi\":true,\"purpose\":1}}";

          m_json_string2 = "{\"cmd\":99, \
                            \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                            \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                            \"conditions\":{\"brand\":\"Nokia\"},\"fields\":{\"category\":1,\"brand\":1}, \
                            \"upsert\":false,\"multi\":true,\"purpose\":1}}";
          m_json_write = "{\"cmd\":102,\"body\":{\"purpose\":2}, \
                          \"docs\":[{\"query\":{\"id\":{\"$oid\":\"%s\"}}, \
                          \"doc\":{\"brand\":\"updated\",\"status\":1234}, \
                          \"upsert\":false,\"multi\":false}]}";
     }
};

BOOST_FIXTURE_TEST_SUITE(mytest, asio_processor_test);

BOOST_AUTO_TEST_CASE(test_parse_request)
{
     BSONObj tmpObj = fromjson(m_json_string);
     BSONObj obj;
     Command cmd=ErrorCmd;

     asio_processor pro(m_json_string.c_str());
     BOOST_CHECK_EQUAL(pro.parse_request(obj,cmd),OK);


     BOOST_CHECK_EQUAL(obj.getIntField("port"),27017);
     BOOST_CHECK_EQUAL(obj.getStringField("host"),"192.168.1.86");
     BOOST_CHECK_EQUAL(obj.getStringField("database"),"zbroker");
     vector<string> docs;
     for( int i =0; i< 500; i++ ){
          docs.push_back(m_json_string);
     }

     request_packet packet;
     pro.pack_response(packet,OK,docs);
     asio_processor pro2(m_json_string2.c_str());
     BOOST_CHECK_EQUAL(pro2.parse_request(obj,cmd),UnknownCommand);
}
BOOST_AUTO_TEST_CASE(test_pack_response)
{
     request_packet packet;
     memset(&packet,0,sizeof(packet));
     asio_processor pro(m_json_string.c_str());
     pro.pack_response(packet,OK );
     BOOST_CHECK(string(packet.data()).find("\"response\" : 200")!=string::npos);
     BOOST_CHECK_EQUAL(pro.pack_response(packet,UnknownCommand),36);
     BOOST_CHECK(string(packet.data()).find("\"response\" : 500")!=string::npos);
}
BOOST_AUTO_TEST_CASE(test_process_read)
{

     BSONObj obj = fromjson(m_json_open);
     const int docs_count = 888;
     broker test_bk;
     BSONObj objBody = obj.getObjectField("body");
     test_bk.open(&objBody);
     test_bk.get_connection().dropCollection("zbroker.broker");

     char buffer[1024];
     const char* json = "{\"brand\":\"Nokia\",\"category\":\"Category_%d\"}";
     for( int i =0 ; i< docs_count ;i++){
          sprintf(buffer,json,i);
          test_bk.get_connection().insert(test_bk.get_docset(),fromjson(buffer));
     }
     BOOST_CHECK_EQUAL( test_bk.query().size() , 888);

     asio_processor pro_no_data(m_json_read_no_data.c_str());
     string res = pro_no_data.process(m_json_open);
     BOOST_CHECK(string(res).find("\"response\" : 503")==string::npos);
     pro_no_data.term();

     asio_processor pro(m_json_read.c_str());
     res = pro.process(m_json_open);
     res = pro.process(m_json_read);

     request_packet packet(res.c_str(),res.size());
     BOOST_CHECK(res.find("\"response\" : 200")!=string::npos);

     bool found = false;
     size_t count = 0;
     while( res.find("\"response\" : 503")==string::npos){
          request_packet packet(res.c_str(),res.size());
          BSONObj tmp = fromjson(packet.body());
          BSONElement e = tmp.getField("docs");
          BOOST_CHECK_EQUAL(e.type(),Array);
          vector<string> tmp_docs;
          e.Obj().Vals(tmp_docs);
          count += tmp_docs.size();
          if(tmp_docs.back().find("Category_887") != string::npos ){
               found = true;
          }
          res = pro.process(m_json_read);
     }
     BOOST_CHECK(found);
     BOOST_CHECK(res.find("\"response\" : 503")!=string::npos);
     BOOST_CHECK_EQUAL(count,888);
     pro.term();
}
BOOST_AUTO_TEST_CASE(test_process_write)
{
     BSONObj obj = fromjson(m_json_open);
     const int docs_count = 888;
     broker test_bk;
     BSONObj objBody = obj.getObjectField("body");
     test_bk.open(&objBody);
     test_bk.get_connection().dropCollection("zbroker.broker");

     char buffer[1024];
     const char* json = "{\"brand\":\"Nokia\",\"category\":\"Category_%d\"}";
     for( int i =0 ; i< docs_count ;i++){
          sprintf(buffer,json,i);
          test_bk.get_connection().insert(test_bk.get_docset(),fromjson(buffer));
     }

     asio_processor pro_no_data(m_json_read_no_data.c_str());
     string res = pro_no_data.process(m_json_open);
     BOOST_CHECK(string(res).find("\"response\" : 503")==string::npos);
     pro_no_data.term();

     asio_processor pro(m_json_read.c_str());
     res = pro.process(m_json_open);
     res = pro.process(m_json_read);

     request_packet packet(res.c_str(),res.size());
     BOOST_CHECK(res.find("\"response\" : 200")!=string::npos);

     bool found = false;
     size_t count = 0;
     while( res.find("\"response\" : 503")==string::npos){
          request_packet packet(res.c_str(),res.size());
          BSONObj tmp = fromjson(packet.body());
          BSONObj arr = tmp.getObjectField("docs");
          vector<string> tmp_docs;
          arr.Vals(tmp_docs);
          count += tmp_docs.size();
          cout << "count:" << count <<endl;
          for( int i =0 ; i< tmp_docs.size() ; i++ ){
               cout << "_id:" << tmp_docs[i] << endl;
          }
          res = pro.process(m_json_read);
     }
     BOOST_CHECK(res.find("\"response\" : 503")!=string::npos);
     BOOST_CHECK_EQUAL(count,888);
     pro.term();
}
BOOST_AUTO_TEST_SUITE_END();

/*
 * vim:ts=5:sw=5:
 */

