#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE request_handler_test
#include <iostream>
#include "common.h"
#include <boost/test/unit_test.hpp>

using namespace std;
using namespace zbroker;
using namespace mongo;

// test fields
BOOST_AUTO_TEST_CASE(test_hash)
{
     boost::hash<std::string> string_hash;
     size_t h = string_hash("test");
     cout << "hash: " << h  << endl;

     h = string_hash("test1");
     cout << "hash: " << h  << endl;

     h = string_hash("1");
     cout << "hash: " << h  << endl;
}
BOOST_AUTO_TEST_CASE(test_handler)
{
     zmq::context_t context (1);
     request_handler rh(&context,"test/config.yml");
     rh.init();
     BOOST_CHECK_EQUAL(rh.get_endpoint(),"tcp://0.0.0.0:5566");

}

/*
 * vim:ts=5:sw=5:
 */

