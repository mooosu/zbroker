#include "common.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE config_test
#include <boost/test/unit_test.hpp>
using namespace mongo;
using namespace std;
BOOST_AUTO_TEST_CASE(test_config_fields)
{
     const char* config_file="test/config.yml";
     broker_config config;
     zxlib::load_yaml(config_file,config);
     BOOST_CHECK_EQUAL(config.mongo_host,"192.168.1.86");
     BOOST_CHECK_EQUAL(config.mongo_port,27017);
     BOOST_CHECK_EQUAL(config.broker_listen_ip,"0.0.0.0");
     BOOST_CHECK_EQUAL(config.broker_listen_port,5566);
}
