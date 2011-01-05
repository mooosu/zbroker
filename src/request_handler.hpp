#ifndef _REQUEST_HANDLER_
#define _REQUEST_HANDLER_
class request_handler{
     private:
          broker_config m_config;
          string m_config_file;
          string m_endpoint;
          zmq::context_t *m_context;
          zmq::socket_t * m_socket;
     protected:
#ifndef BOOST_TEST_MODULE
          void init();
#endif
     public:
          request_handler(zmq::context_t *context ,const char* config_file);
          ~request_handler()
          {
               if( m_socket ){
                    delete m_socket;
               }
          }
          void run();
#ifdef BOOST_TEST_MODULE
          void init();
#endif
          void handler(zmq::message_t& request);
          string get_config_file() { return m_config_file;}
          string get_endpoint() { return m_endpoint; }

          void pack_reply(std::string& doc, zmq::message_t& reply);

};
#endif
