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

