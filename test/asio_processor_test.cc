#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE asio_processor_test

#include "common.h"
#include "asio_processor.hpp"
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
     string m_json_open_write;
     asio_processor_test(){
          m_json_string = "{\"cmd\":100, \
                           \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                           \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                           \"conditions\":{\"brand\":\"Nokia\"}, \"fields\":{\"category\":1,\"brand\":1,\"status\":1}, \
                           \"upsert\":false,\"multi\":true,\"purpose\":1}}";
          m_json_open  = m_json_string;
          m_json_read = "{\"cmd\":102, \
                         \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                         \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                         \"conditions\":{\"brand\":\"Nokia\"}, \"fields\":{\"category\":1,\"brand\":1}, \
                         \"upsert\":false,\"multi\":true,\"purpose\":1}}";
          m_json_read_no_data = "{\"cmd\":102, \
                                 \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                                 \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                                 \"conditions\":{\"brand\":\"123123123\"}, \"fields\":{\"category\":1,\"brand\":1}, \
                                 \"upsert\":false,\"multi\":true,\"purpose\":1}}";

          m_json_string2 = "{\"cmd\":99, \
                            \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                            \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                            \"conditions\":{\"brand\":\"Nokia\"},\"fields\":{\"category\":1,\"brand\":1}, \
                            \"upsert\":false,\"multi\":true,\"purpose\":1}}";

          m_json_open_write = "{\"cmd\":100, \
                           \"body\":{\"host\":\"192.168.1.86\",\"port\":27017,\"database\":\"zbroker\", \
                           \"collection\":\"broker\",\"parameters\":{\"skip\":0,\"limit\":1000, \"queue_size\":100}, \
                           \"conditions\":{\"brand\":\"Nokia\"}, \"fields\":{\"category\":1,\"brand\":1,\"status\":1}, \
                           \"upsert\":false,\"multi\":true,\"purpose\":2}}";

          m_json_write = "{\"cmd\":103,\"body\":{\"purpose\":2, \
                          \"docs\":[{\"query\":{\"_id\":{\"$oid\":\"%s\"}}, \
                          \"doc\":{\"brand\":\"Nokia\",\"status\":\"%s\"}, \
                          \"upsert\":false,\"multi\":false}]}}";
     }
};

BOOST_FIXTURE_TEST_SUITE(mytest, asio_processor_test);

BOOST_AUTO_TEST_CASE(test_parse_request)
{
     BSONObj tmpObj = fromjson(m_json_string);
     BSONObj obj;
     Command cmd=ErrorCmd;

     asio_processor pro("xxx");
     BOOST_CHECK_EQUAL(asio_processor::parse_request(obj,cmd,m_json_string),OK);


     BOOST_CHECK_EQUAL(obj.getIntField("port"),27017);
     BOOST_CHECK_EQUAL(obj.getStringField("host"),"192.168.1.86");
     BOOST_CHECK_EQUAL(obj.getStringField("database"),"zbroker");
     vector<string> docs;
     for( int i =0; i< 500; i++ ){
          docs.push_back(m_json_string);
     }

     out_packet packet;
     pro.pack_response(packet,OK,docs);
     asio_processor pro2("xxx");
     BOOST_CHECK_EQUAL(asio_processor::parse_request(obj,cmd,m_json_string2),UnknownCommand);
}
BOOST_AUTO_TEST_CASE(test_pack_response)
{
     out_packet packet;
     asio_processor pro("xxx");

     asio_processor::pack_response(packet,OK );
     BOOST_CHECK(string(packet.data()).find("\"response\" : 200")!=string::npos);

     asio_processor::pack_response(packet,UnknownCommand);
     BOOST_CHECK_EQUAL(packet.length(),68);
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
     BOOST_CHECK_EQUAL( test_bk.query().size()+test_bk.size(), 888);

     cout << "should open processor" << endl;

     asio_processor pro_no_data("xxx");
     string res = pro_no_data.process(m_json_open);
     BOOST_CHECK(string(res).find("\"response\" : 503")==string::npos);
     BOOST_CHECK(string(res).find("\"response\" : 200")!=string::npos);
     pro_no_data.term();


     cout << "should read processor" << endl;

     asio_processor pro("xxx");
     res = pro.process(m_json_open);
     res = pro.process(m_json_read);

     BOOST_CHECK(res.find("\"response\" : 200")!=string::npos);

     bool found = false;
     size_t count = 0;
     while( res.find("\"response\" : 503")==string::npos){
          in_packet packet(res.c_str(),res.size());
          BSONObj tmp = fromjson(packet.body());
          BSONElement e = tmp.getField("docs");
          BOOST_CHECK_EQUAL(e.type(),Object);
          vector<BSONObj> tmp_docs;
          e.Obj().Vals(tmp_docs);
          count += tmp_docs.size();
          cout <<"140: "<<  tmp_docs.back().jsonString()<< endl;
          for( int i = 0 ; i< tmp_docs.size() ; i++ ){
               if(tmp_docs[i].jsonString().find("Category_887") != string::npos ){
                    found = true;
                    break;
               }
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

     asio_processor pro_no_data("xxx");
     string res = pro_no_data.process(m_json_open);
     BOOST_CHECK(string(res).find("\"response\" : 503")==string::npos);
     pro_no_data.term();

     asio_processor pro("xxx");
     res = pro.process(m_json_open);
     res = pro.process(m_json_read);

     in_packet packet(res.c_str(),res.size());
     BOOST_CHECK(res.find("\"response\" : 200")!=string::npos);

     bool found = false;
     size_t count = 0;
     vector<string> update_queries;
     char tmp_buffer[1024];
     while( res.find("\"response\" : 503")==string::npos){
          in_packet packet(res.c_str(),res.size());
          BSONObj tmp = fromjson(packet.body());
          BSONObj e  = tmp.getObjectField("docs");
          vector<BSONElement> tmp_docs;
          e.elems(tmp_docs);

          count += tmp_docs.size();
          for( int i =0 ; i< tmp_docs.size() ; i++ ){
               BSONElement id;
               tmp_docs[i].Obj().getObjectID(id);
               sprintf(tmp_buffer,m_json_write.c_str(),id.OID().str().c_str(),id.OID().str().c_str());
               update_queries.push_back(tmp_buffer);
          }
          res = pro.process(m_json_read);
     }

     BOOST_CHECK(res.find("\"response\" : 503")!=string::npos);
     BOOST_CHECK_EQUAL(count,888);
     BOOST_CHECK_EQUAL(update_queries.size(),888);
     pro.term();

     cout << "should write processor" << endl;

     asio_processor pro_write("xxx");
     try{
          res = pro_write.process(m_json_open_write);
          BSONObj o = fromjson(update_queries[0]);
          for( int i =0 ; i< update_queries.size() ; i++ ){
               res = pro_write.process(update_queries[i]);
          }
     } catch ( std::exception &ex){
          cout << "msg:" << ex.what() << endl;
     }
     pro_write.term();

     obj = fromjson(m_json_open);
     broker verify_write;
     objBody = obj.getObjectField("body");
     verify_write.open(&objBody);
     vector<string> written_docs = verify_write.query();
     BOOST_CHECK_EQUAL(written_docs.size() + verify_write.size() , 888);
     for( int i =0 ; i< written_docs.size() ; i++ ){
          BSONObj tmp= fromjson(written_docs[i]);
          BSONElement id;
          tmp.getObjectID(id);
          BOOST_CHECK_EQUAL(tmp.getStringField("status"),id.OID().str());
     }
}
BOOST_AUTO_TEST_SUITE_END();

/*
 * vim:ts=5:sw=5:
 */

