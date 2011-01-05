#include "common.h"

#include <boost/functional/hash.hpp>
#include <google/sparse_hash_map>
request_handler::request_handler(zmq::context_t *context ,const char* config_file)
{
     m_config_file = config_file;
     m_context  = context;
}
void request_handler::run()
{
     init();
     m_socket = new zmq::socket_t(*m_context,ZMQ_REP);
     m_socket->bind(m_endpoint.c_str());
    while (true) {
        zmq::message_t request;

        //  Wait for next request from client
        m_socket->recv (&request);

        handler(request);
    }
}
void request_handler::init()
{
     zxlib::load_yaml(m_config_file.c_str(),m_config);
     char *tmp = new char[m_config.broker_listen_ip.size()+64];
     sprintf(tmp,"tcp://%s:%d",m_config.broker_listen_ip.c_str(),m_config.broker_listen_port);
     m_endpoint  = tmp;
     delete tmp;
}
void request_handler::handler(zmq::message_t& request)
{
     zmq::message_t reply;
     pack_reply(doc,reply,OK);
     m_socket->send(reply);
}

void request_handler::pack_reply(std::string& doc, zmq::message_t& reply){
     unsigned int irep = (int)rep;
     size_t irep_size = sizeof( irep );
     reply.rebuild(doc.size()+irep_size);
     memcpy(reply.data(), doc.c_str(), doc.size());
     memcpy(((char*)reply.data()+doc.size()),&rep,irep_size);
}
