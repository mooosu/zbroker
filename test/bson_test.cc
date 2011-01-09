#include "client/dbclient.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE bson_test
#include <boost/test/unit_test.hpp>
using namespace mongo;
using namespace std;
BOOST_AUTO_TEST_CASE(test_fields)
{

     BSONObjBuilder builder;
     BSONObj new_conditions;
     BSONObj conditions = fromjson("{\"conditions\":{\"brand\":\"Nokia\"}}");
     BSONObj conditions2 = fromjson("{\"conditions\":{\"brand\":\"Nokia\"}}");
     cout << "conditions md5: " << conditions.md5() << endl;
     cout << "conditions2 md5: " << conditions2.md5() << endl;
     new_conditions = builder.appendElements(conditions).append("_id",BSONObjBuilder().append("$gt",string("4d22afbfe401195ff7785332")).obj()).obj();
}
BOOST_AUTO_TEST_CASE(test_oid)
{
     OID id ;
     id.init("4d22afbfe401195ff7785332");

     BSONElement e;
     char buffer[1024];
     sprintf(buffer,"{ \"_id\" : { \"$oid\" : \"%s\" }}","4d22afbfe401195ff7785332");
     BSONObj bo = fromjson(buffer);
     e = bo.getField("_id");
     BOOST_CHECK_EQUAL(e.OID().str(),id.str());
     BOOST_CHECK_EQUAL(e.type(),jstOID);
}
BOOST_AUTO_TEST_CASE(test_bool )
{
     BSONObj obj = fromjson("{\"iv1\":123,\"upsert\":false,\"multi_int\":1,\"multi\":true}");
     BOOST_CHECK_EQUAL(obj.getBoolField("iv1"),false);
     BOOST_CHECK_EQUAL(obj.getBoolField("iv2"),false);
     BOOST_CHECK_EQUAL(obj.getBoolField("upsert"),false);
     BOOST_CHECK_EQUAL(obj.getBoolField("multi_int"),false);
     BOOST_CHECK_EQUAL(obj.getBoolField("multi"),true);
}
