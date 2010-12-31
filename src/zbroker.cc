//g++ mongohlp.cc -lboost_program_options
//
#include <zmq.hpp>
#include <unistd.h>
#include <iostream>
#include "common.h"
using namespace std;
using namespace mongo;
typedef struct{
     const char* msg;
     size_t size;
}error_message;
#define MAX_REQUEST_MESSAGE_SIZE  (1024*1024)
#define ERROR_MESSAGE(x) {x,sizeof(x)-1}
static error_message error_messages[]={
     ERROR_MESSAGE( "Command not found"),
     ERROR_MESSAGE( "Command unimplemented yet!"),
     ERROR_MESSAGE( "Request body exceed limit(1MB)!"),
     ERROR_MESSAGE( "No more items!"),
     ERROR_MESSAGE( "Internal Error(invalid error code)")
};
class processor{
     typedef enum {
          OPEN  = 100,
          READ  = 101,
          WRITE = 102
     }Command;
     typedef enum {
          MinError       = 500,
          UnknownCommand = 500,
          Unimplemented  = 501,
          RequestTooLong = 502,
          NoMoreItem     = 503,
          ErrorMax = NoMoreItem+1,
          OK             = 200,
     }Response;
     private:
          zbroker::broker m_read_broker;
          zbroker::broker m_update_broker;
          zmq::socket_t *m_socket;
          char* m_request_buf;
          size_t m_request_buf_size;
          
     public:
          processor(zmq::socket_t *socket ,const char* docset_name, size_t size, string doc_server ,
                    int port=27017 ,size_t batch_size=100)
          {
               m_socket = socket;
               m_read_broker.open(BSONObj());
               m_update_broker.open(BSONObj());
               m_request_buf = NULL;
               m_request_buf_size = 0;
          }
          zbroker::broker* read_broker(){ return &m_read_broker;}
          zbroker::broker* update_broker(){ return &m_update_broker;}
          void pack_reply(std::string& doc, zmq::message_t& reply,Response rep ){
               unsigned int irep = (int)rep;
               size_t irep_size = sizeof( irep );
               reply.rebuild(doc.size()+irep_size);
               memcpy(reply.data(), doc.c_str(), doc.size());
               memcpy(((char*)reply.data()+doc.size()),&rep,irep_size);
          }
          void send_reply( )
          {
               try{
                    std::string doc = m_read_broker.pop(1);
                    zmq::message_t reply;
                    pack_reply(doc,reply,OK);
                    m_socket->send(reply);
               }catch( zbroker::broker_timeout &e ){
                    cout << "timeout: " << e.what() << endl; 
                    send_error(NoMoreItem);
               }
          }
          void send_error( Response error ){
               if(error < ErrorMax && error> MinError ){
                    int err_no = error - MinError;
                    error_message* msg = &error_messages[err_no];
                    zmq::message_t reply (msg->size);
                    memcpy ((void *) reply.data(), msg->msg, msg->size);
                    m_socket->send(reply);
               }
          }
          char* request_data( zmq::message_t& request ){
               if( request.size() > m_request_buf_size){
                    if(m_request_buf) free(m_request_buf);
                    m_request_buf_size = request.size();
                    m_request_buf = (char*)malloc( m_request_buf_size+1);
               } 
               memcpy(m_request_buf,request.data(),m_request_buf_size);
               m_request_buf[request.size()]=0;
               printf ("Received request: [%s]\n", m_request_buf);
               return m_request_buf;
          }
          void process( zmq::message_t& request )
          {
               if( request.size() > MAX_REQUEST_MESSAGE_SIZE ){
                    send_error( RequestTooLong );
                    return;
               }
               BSONObj cmdObj=fromjson(request_data(request));
               Command cmd = (Command)cmdObj.getIntField("cmd");
               switch( cmd ){
                    case READ:
                         send_reply();
                         break;
                    case WRITE:
                         send_error( Unimplemented);
                         break;
                    default:
                         send_error( UnknownCommand );
               }

          }
};
void *read_queue_thread(void *arg)
{
     processor *pro= (processor *) arg;
     pro->read_broker()->read();
     return (NULL);
}
void* update_queue_thread( void *arg )
{
     processor *pro= (processor *) arg;
     pro->update_broker()->update();
     return (NULL);
}
int main () {
    //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind ("tcp://*:5666");
    processor pro(&socket,"zbot_development.download",100,"192.168.1.86",7890,100);

    pthread_t worker;
    pthread_create (&worker, NULL, read_queue_thread, (void *) &pro);
    pthread_create (&worker, NULL, update_queue_thread, (void *) &pro);
    while (true) {
        zmq::message_t request;

        //  Wait for next request from client
        socket.recv (&request);
        pro.process( request);

    }
    return 0;
}

