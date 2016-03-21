#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>
#include "MQTTClient.h"

//#define DEBUG 1

#define ADDRESS     	"tcp://10.10.10.1:1883"
#define CLIENTID    	"AWGESDDD"
#define TOPIC_SUB   	"lights/+/status"
#define QOS         1
#define TIMEOUT     10000L
#define HELLO       "Hello AWGES DEV:"

#ifdef DEBUG
#define DBG printf
#else
#define DBG(format, args...) ((void)0)
#endif

MQTTClient client;
MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

MQTTClient_deliveryToken token;
int rc;
int i, j;

pthread_mutex_t mux;

typedef struct {
	int id;
	char mac [8];
	char command_dimmer[9];
	char topico [30];
	}light_status;

pthread_t lamps[700];
pthread_t connections[700];

light_status light_luminary[700];

void* publish_retentive(void *arg)
{
	pthread_mutex_lock(&mux);

	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	srand(time(NULL));
	usleep(300000);
	light_status* temp = (light_status*)arg;
	pubmsg.payload = temp->command_dimmer;
    pubmsg.payloadlen = 9;
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    MQTTClient_publishMessage(client, temp->topico, &pubmsg, &token);
	DBG("temp->command_dimmer: %s\n temp->topico: %s\n\n", temp->command_dimmer, temp->topico);
    
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    
	MQTTClient_unsubscribe(client, temp->topico);
	DBG("Unsubscribe: %s\n",temp->topico);
	
	 pthread_mutex_unlock(&mux);
	return NULL;
}

void* subscrib_new_topic(void *arg)
{
	
	char topico[15];
	memcpy(topico,arg,15);
	topico[15] = '\0';
	
	pthread_mutex_lock(&mux);
	DBG("subscrib_new_topic on thread: %s\n",topico);
	MQTTClient_subscribe(client, topico, QOS);
	DBG("MQTTClient_subscribe\n");
	pthread_mutex_unlock(&mux);
}
volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    DBG("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    char mac[8];
    char new_topic[30];
    char* payloadptr;
    int err;
    
        memcpy(&mac[0],&topicName[7],8);
	mac[8]='\0';

    DBG("topic: %s\n", topicName);
	DBG("MAC: %s\n",mac);
    payloadptr = message->payload;
    

	sprintf(new_topic,"lights/%s",mac);
	DBG("MQTTClient_subscribe new topic: %s\n",new_topic);
	//verifica se é /lights/MACxxxxx/status e se possui string HELLO na mensagen
	if (strlen(topicName)>16 && strstr(message->payload,HELLO)!=NULL)
    { 		
		if(j<700)
		{
			err = pthread_create(&(connections[j]), NULL, &subscrib_new_topic, (void*)new_topic);
			if (err != 0)
				DBG("\ncan't create thread :[%s]", strerror(err));
			else
				DBG("\n Thread created successfully\n");
				usleep(100);
			j++;
		}
		else j =0;
	}
	else{
		if ( *(char*)(message->payload) == 'W')
		{
			DBG("Starting The thread\n");
			if(i<700)
			{
				light_luminary[i].id = i;
				
				strcpy(light_luminary[i].command_dimmer,message->payload);
				strcpy(light_luminary[i].topico,new_topic);
				err = pthread_create(&(lamps[i]), NULL, &publish_retentive, (void*)&light_luminary[i]);
				if (err != 0)
					DBG("\ncan't create thread :[%s]", strerror(err));
				else
					DBG("\n Thread created successfully\n");
				i++;
			}
			else i=0;
		 }
    }
	DBG("MQTTClient_freeMessage(&message);\n");
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    DBG("\nConnection lost\n");
    DBG("     cause: %s\n", cause);
}

int main(void)
{
	int i = 0, ch;
    int err;
    
    pid_t process_id = 0;
	pid_t sid = 0;

	process_id = fork();

	if (process_id < 0)
	{
		printf("fork failed!\n");
		exit(1);
	}

	if (process_id > 0){
		printf("process_id of child process %d \n", process_id);
		exit(0);
	}

	sid = setsid();
	if(sid < 0)
		exit(1);
    
     MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        DBG("Failed to connect, return code %d\n", rc);
        exit(-1);
    } 

    DBG("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC_SUB, CLIENTID, QOS);
           
    MQTTClient_subscribe(client, TOPIC_SUB, QOS);

   for(;;){
   sleep(10);
   }

    return 0;
}
