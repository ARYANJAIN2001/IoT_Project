#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTClient.h"
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#define ADDRESS     "tcp://localhost:1883"
#define CLIENTID    "ExampleClientSub"
#define SUB_TOPIC   "powerSave/analysis"
#define PUB_TOPIC   "powerSave/alert"
#define QOS         1
#define TIMEOUT     10000L
volatile MQTTClient_deliveryToken deliveredtoken;
MQTTClient client;
volatile long long last_updated_time = -1;
int last_state=-1; 
sem_t mutex;
FILE *fpt;

long long timeInMilliseconds(void) {
    struct timeval tv;

    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}


void* publishThread(void * arg){
    
    while(1){
        sem_wait(&mutex);
        if(last_updated_time!=-1){
           if((timeInMilliseconds()-last_updated_time)>60000LL){
            MQTTClient_message pubmsg = MQTTClient_message_initializer;
            MQTTClient_deliveryToken token;
            pubmsg.payload = "inactive";
            pubmsg.payloadlen = strlen("inactive");
            pubmsg.qos = QOS;
            pubmsg.retained = 0;
            printf("inactive\n");
            deliveredtoken = 0;
            MQTTClient_publishMessage(client, PUB_TOPIC, &pubmsg, &token);
            last_updated_time = timeInMilliseconds();
            }
        }
        sem_post(&mutex);
    }
    
}
void delivered(void *context, MQTTClient_deliveryToken dt)
{
   // printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;
    // printf("Message arrived\n");
    // printf("     topic: %s\n", topicName);
    // printf("   message: ");

    payloadptr = message->payload;
    int code = (int)(*payloadptr);
    printf("Mode of the Led: %d\n",code);
    if(code>=8){
        sem_wait(&mutex);
         last_updated_time = -1;
         last_state =-1;
         sem_post(&mutex);
    }else{
        
        if(last_updated_time==-1){
            sem_wait(&mutex);
            last_updated_time = timeInMilliseconds();
            last_state = code;
            sem_post(&mutex);
        }else{
           if(last_state!=code){
            sem_wait(&mutex);
             last_updated_time = timeInMilliseconds();
             last_state = code;
             sem_post(&mutex);
           }   
        }
    }
   time_t ltime;
   time(&ltime);
   fpt = fopen("Power_State.csv", "a"); 
    if(code%2==0){
        
        fprintf(fpt,"%d, %s",0,ctime(&ltime));
        
    }else{
        fprintf(fpt,"%d, %s",1,ctime(&ltime));
    }
    fclose(fpt);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}
int main(int argc, char* argv[])
{
   
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;
    sem_init(&mutex, 0, 1);
    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
    // printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
    //        "Press Q<Enter> to quit\n\n", SUB_TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, SUB_TOPIC, QOS);
  

    fpt = fopen("Power_State.csv", "w+");
    fprintf(fpt,"powerState, time\n");
    fclose(fpt);
    pthread_t pub_thread;
    pthread_create(&pub_thread,NULL,publishThread,NULL);
    // do
    // {
    //     ch = getchar();
    // } while(ch!='Q' && ch != 'q');

    pthread_join(pub_thread,NULL);
    
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}
