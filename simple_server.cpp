/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include<stdbool.h>
#include<limits.h>
#include<signal.h>
#include <netinet/in.h>

#include <pthread.h>

#include "http_server.cpp"
//#include "http_server.hh"



void error(const char* msg){
    perror(msg);
    exit(0);
}

#define THREAD_POOL_SIZE 100

int flag;

pthread_t thread_pool[THREAD_POOL_SIZE]; //
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;


node_t* head=NULL;
node_t* tail=NULL;

void enque(int *client_socket){
   node_t *newnode= new node_t;
    newnode->client_socket=client_socket;
    newnode->next=NULL;
    if(tail==NULL){
        head=newnode;
    }else{
        tail->next=newnode;
    }
    tail=newnode;
}

int* deque(){
    if(head==NULL){
        return NULL;
    }else{
        int *result=head->client_socket;
        node_t *temp=head;
        head=head->next;
        if(head==NULL){
            tail=NULL;
        }
        delete temp;//modification we replace free with delete
        return result;
    }
    return 0;
}

void *handle_connection(void * p_client_socket);

void * thread_function(void *arg){
    while(true){
                 int *pclient;
                 pthread_mutex_lock(&mutex);
                 if((pclient=deque())==NULL){
                        pthread_cond_wait(&condition_var,&mutex);
                        //try again
                        if(flag==1){
                            pthread_mutex_unlock(&mutex);
                            pthread_exit(NULL);
                        }
                        pclient=deque();
                        //handle_connection(pclient);
        
                 }
                 pthread_mutex_unlock(&mutex);
                 if(pclient!=NULL){//check krna hai NULL bhi ho skta hai
                        handle_connection(pclient);
                  
                 }
        }
}
void  signal_handler(int sig)
{
  flag=1;
  pthread_cond_broadcast(&condition_var);
  for(int i=0;i<THREAD_POOL_SIZE;i++)
  {
    pthread_cancel(thread_pool[i]);
    pthread_join(thread_pool[i],NULL);
  }
  exit(0);
}


void *handle_connection(void *p_client_socket){
    char buffer[255];
    int n;
    int newsockfd=*((int *)p_client_socket);
    free(p_client_socket);
    bzero(buffer,255);
    n=read(newsockfd,buffer,255);
    if(n==0)
      return NULL;
    if(n<0) 
        error("Error on reading");

    HTTP_Response *temp=handle_request(buffer);   

    string s=string(temp->get_string());
    char buffer1[s.size() + 1];
    strcpy(buffer1, s.c_str());
    n=write(newsockfd,buffer1,strlen(buffer1));
    cout<<buffer1<<endl;
    bzero(buffer1,strlen(buffer1));
    // sleep(50);
    free(temp);
    if(n<0)
        error("error on write:");
    close(newsockfd);
    return NULL;

}

int portno;

int main(int argc, char * argv[]){

    if(argc<2){
        fprintf(stderr,"port no not provided program terminated\n");
        exit(1);
    }
    portno=atoi(argv[1]);

    for(int i=0;i<THREAD_POOL_SIZE;i++){
              pthread_create(&thread_pool[i],NULL,thread_function,NULL);
        }

   int sockfd,newsockfd,n;
    char buffer[255];
    struct sockaddr_in serv_addr,cli_addr;
    socklen_t clilen;
    sockfd=socket(AF_INET,SOCK_STREAM,0);
    if(sockfd<0)
         error("Error opening socket");

    bzero((char *)&serv_addr,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=INADDR_ANY;
    serv_addr.sin_port=htons(portno);
    

    if(bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0)
        error("Binding failed\n");
    listen(sockfd,100);
    clilen=sizeof(cli_addr);    

    while(1){
        //signal handler
        signal(SIGINT,signal_handler);
        newsockfd=accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        if(newsockfd<0)
          error("Error  on Accept\n");
    
        int *pclient=new int;
        *pclient=newsockfd;
        pthread_mutex_lock(&mutex);
        enque(pclient); 
        pthread_cond_signal(&condition_var);
        pthread_mutex_unlock(&mutex);
    }

    // for( int i=0;i<THREAD_POOL_SIZE;i++){
    //     pthread_join(thread_pool[i],NULL);
    // }
    return 0;   
    
}
