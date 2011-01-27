#ifndef _ZBROKER_PROTOCOL_H_
#define _ZBROKER_PROTOCOL_H_

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
typedef enum {
     ErrorCmd = -1,
     MinCmd = 100,
     OPEN  = 100,
     CLOSE = 101,
     READ  = 102,
     WRITE = 103,
     REWIND = 106,
     MaxCmd = REWIND
}Command;
typedef enum{
     Read = 1,
     Write = 2
}Purpose;
typedef enum {
     MinError         = 500,
     UnknownCommand   = 500,
     Unimplemented    = 501,
     RequestTooLong   = 502,
     NoMoreItem       = 503,
     ResponseTooLong  = 504,
     AlreadyOpen      = 505,
     ErrorMax = AlreadyOpen+1,
     OK               = 200,
}Response;

#endif
