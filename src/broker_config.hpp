#ifndef _BROKER_CONFIG_H_
#define _BROKER_CONFIG_H_

#include "texting/ztexting.h"

typedef struct _broker_config:zxlib::zconfig_yaml {
     string mongo_host;
     int mongo_port;
     string broker_listen_ip;
     int broker_listen_port;
     void set_config(const YAML::Node& node )
     {
          const YAML::Node& mongo = node["mongo"];
          const YAML::Node& broker = node["broker"];

          mongo["host"]>> mongo_host;
          mongo["port"]>> mongo_port;

          broker["listen_ip"] >> broker_listen_ip;
          broker["listen_port"] >> broker_listen_port;
     }
     void inspect()
     {
          cout << "mongo_host:" << mongo_host << endl;
          cout << "mongo_port:" << mongo_port << endl;
          cout << "broker_listen_ip:" << broker_listen_ip << endl;
          cout << "broker_listen_port:" << broker_listen_port << endl;
     }

}broker_config;

#endif
