/* run using ./server <port> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstring>
#include <unistd.h>
#include<stdbool.h>
#include<limits.h>
#include <signal.h>
#include <netinet/in.h>

#include <pthread.h>

#include <vector>
#include<queue>

#include <sys/stat.h>

#include <fstream>
#include <sstream>
#define _HTTP_SERVER_HH_

#define  MYQUEUE_H_

#include<iostream>
using namespace std;

struct HTTP_Request {
  string HTTP_version;

  string method;
  string url;

  HTTP_Request(string request);
};

struct HTTP_Response {
  string HTTP_version;

  string status_code; 
  string status_text; 

  string content_type;
  string content_length;

  string body;

  string get_string(); 
};

HTTP_Response *handle_request(string request);


vector<string> split(const string &s, char delim) {
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim)) {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}

HTTP_Request::HTTP_Request(string request) {
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; 

  this->method=first_line[0];
  this->url=first_line[1];
  if (this->method != "GET") {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}
 
  HTTP_Response *handle_request(string req) {
  HTTP_Request *request = new HTTP_Request(req);
  HTTP_Response *response = new HTTP_Response();
  string url = string("html_files") + request->url;
  response->HTTP_version = "1.0";

  struct stat sb;
  if (stat(url.c_str(), &sb) == 0)
  {
    response->status_code = "200";
    response->status_text = "OK";
    response->content_type = "text/html";
    string body;

    if (S_ISDIR(sb.st_mode)) {
      url=url+"index.html";

    }
    std::ifstream inFile;
    inFile.open(url);
    std::stringstream strStream;
    strStream << inFile.rdbuf(); 
    std::string str = strStream.str();
    body=str;
    response->content_length=std::to_string(body.size());
    response->body=body;
  }


  else {
    response->status_code = "404";
    response->status_text = "Not Found";
    response->content_type = "text/html";
    string body;
    std::ifstream inFile;
    inFile.open("html_files/html404.html");
    std::stringstream strStream;
    strStream << inFile.rdbuf(); 
    std::string str = strStream.str();
    body=str;
    response->content_length=std::to_string(body.size());
    response->body=body;
  }

  free(request);
  return response;
}

string HTTP_Response::get_string() {
 string result;
 result="HTTP/"+ HTTP_version+" " + status_code+" "+ status_text +"\n"+ "Content-Length: "+content_length +"\n"+ "Content-Type: "+content_type+"\n" + body + "\n\n";
 return result;
}
void error(const char* msg){
    perror(msg);
    exit(0);
}

#define THREAD_POOL_SIZE 100


pthread_t thread_pool[THREAD_POOL_SIZE]; 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;



std::queue<int>que;

int flag;

void handle_connection(int p_client_socket);

void* thread_function(void *arg){
    while(true){
                 int pthread;
                 pthread_mutex_lock(&mutex);
                 if(que.empty()){
                        pthread_cond_wait(&condition_var,&mutex);
                        if(flag==1){
                            pthread_mutex_unlock(&mutex);
                            pthread_exit(NULL);
                        }
                        pthread=que.front();
                        que.pop();
                        handle_connection(pthread);
        
                 }
                 pthread_mutex_unlock(&mutex);
                //  handle_connection(pthread);
  
                //  if(pthread!=que.empty()){//check krna hai NULL bhi ho skta hai
                //         handle_connection(pthread);
                  
                //  }
        }
}



void handle_connection(int newsockfd){
    char buffer[255];
    int n;
    //int newsockfd=p_client_socket;
    bzero(buffer,255);
    n=read(newsockfd,buffer,255);
    if(n==0)
      return ;
    if(n<0) 
        error("Error on reading");

    HTTP_Response *temp=handle_request(buffer);   

    string s=string(temp->get_string());
    char buffer1[s.size() + 100];
    strcpy(buffer1, s.c_str());
    n=write(newsockfd,buffer1,strlen(buffer1));
    cout<<buffer1<<endl;
    bzero(buffer1,strlen(buffer1));
    free(temp);
    if(n<0)
        error("error on write:");
    close(newsockfd);
    return ;

}

int portno;

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
        signal(SIGINT, signal_handler);
        newsockfd=accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        if(newsockfd<0)
          error("Error  on Accept\n");
        pthread_mutex_lock(&mutex);
        que.push(newsockfd); 
        pthread_cond_signal(&condition_var);
        pthread_mutex_unlock(&mutex);
    }
    return 0;   
    
}
