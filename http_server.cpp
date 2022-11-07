#include "http_server.hh"

#include <vector>

#include <sys/stat.h>

#include <fstream>
#include <sstream>


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

  this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

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
  if (stat(url.c_str(), &sb) == 0) // requested path exists
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
