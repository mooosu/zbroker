//g++ mongohlp.cc -lboost_program_options
//
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
          processor(zmq::message_t& request )
          {
               m_request_buf = NULL;
               m_request_buf_size = 0;
               get_request_data(request);
          }

          char* get_request_data( zmq::message_t& request ){
               if( m_request_buf ) free(m_request_buf);
               m_request_buf_size = request.size();
               m_request_buf = (char*)malloc( m_request_buf_size+1);
               memcpy(m_request_buf,request.data(),m_request_buf_size);
               m_request_buf[request.size()]=0;
               printf ("Received request: [%s]\n", m_request_buf);
               return m_request_buf;
          }
          void pack_reply(std::string& doc, zmq::message_t& reply,Response rep ){
               unsigned int irep = (int)rep;
               size_t irep_size = sizeof( irep );
               reply.rebuild(doc.size()+irep_size);
               memcpy(reply.data(), doc.c_str(), doc.size());
               memcpy(((char*)reply.data()+doc.size()),&rep,irep_size);
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
          void process( zmq::message_t& request )
          {
               if( request.size() > MAX_REQUEST_MESSAGE_SIZE ){
                    send_error( RequestTooLong );
                    return;
               }
               BSONObj cmdObj=fromjson(get_request_data(request));
               Command cmd = (Command)cmdObj.getIntField("cmd");
               switch( cmd ){
                    case READ:
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
     return (NULL);
}
void* update_queue_thread( void *arg )
{
     processor *pro= (processor *) arg;
     return (NULL);
}
int main () {
    return 0;
}
