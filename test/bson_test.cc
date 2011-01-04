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
     new_conditions = builder.appendElements(conditions).append("_id",BSONObjBuilder().append("$gt",string("4d22afbfe401195ff7785332")).obj()).obj();
     cout << new_conditions.jsonString();
}

