#ifndef __COMMON_H__
#define __COMMON_H__

#include "client/dbclient.h"
#include <boost/assert.hpp>
#include <glog/logging.h>

#include <zmq.hpp>
#include <unistd.h>
#include <iostream>

#include "broker_config.hpp"

//#include "request_handler.hpp"
//
#include "request_packet.hpp"
#include "color.h"
using std::cout;
using std::string;
using std::vector;

using mongo::OID;
using mongo::Query;
using mongo::fromjson;
using mongo::BSONElement;
using mongo::BSONObj;
using mongo::BSONObjBuilder;
using mongo::DBClientCursor;
using mongo::DBClientConnection;
using zxlib::red_text;
using zxlib::blue_text;
using zxlib::red_begin;
using zxlib::blue_begin;
using zxlib::color_end;
using zxlib::color_id;

#endif

