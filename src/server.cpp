#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <filesystem>
#include <fstream>
#include "ThreadPool.h"
using namespace std; 
#define MAXBUFFER 1024

string ack = "HTTP/1.1 200 OK\r\n\r\n";
string nack_not_found = "HTTP/1.1 404 Not Found\r\n\r\n";
string nack_bad_request = "HTTP/1.1 400 Bad Request\r\n\r\n";

void send_file_tcp(string recvd_data, int client_fd) {
  //Will check by default in the Main directory's TestFiles Folder
      int start_ind = recvd_data.find("/files/")+7;
      int end_ind = recvd_data.find("HTTP")-1;
      string file_name = recvd_data.substr(start_ind, end_ind - start_ind);
      
      string path_to_dir = "./TestFiles";
      string file_path = "";

      for(auto iter : filesystem::directory_iterator(path_to_dir)) {
        string cur_file = iter.path();
        cur_file = cur_file.substr(path_to_dir.length()+1);
        cout<<cur_file<<endl;
        if(cur_file == file_name) {
          file_path = iter.path();
        }
      }

      if(file_path=="") {
        cout<<"NOT FOUND"<<endl;
        send(client_fd, nack_not_found.c_str(), nack_not_found.size(),0);
        return;
      }

      ifstream file_data (file_path, ios::binary);

      if (!file_data.is_open()) {
        std::cerr << "Failed to open file: " << file_path << std::endl;
        std::string errorResponse = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
        send(client_fd, errorResponse.c_str(), errorResponse.size(), 0);
        return;
      }

      size_t file_size = filesystem::file_size(file_path);
      cout<<"Attempting to read "<<file_path<<" "<<file_size<<endl;

      string response_header = "HTTP/1.1 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: "
                       + to_string(file_size) + "\r\n\r\n";
      send(client_fd, response_header.c_str(), response_header.size(),0);

      //Socket might not send everything that we have asked it to so we chunk the file into 1024B
      char data_buffer[MAXBUFFER];
      while(!file_data.eof()) {
        file_data.read(data_buffer,MAXBUFFER);
        //ifstream read will also move pointer ahead after Reading MAXBUFFER bytes. If the data is less
        //than MAXBUFFER EOF bit gets set automatically. gcount for the number of bytes actually read.
        size_t bytes_read = file_data.gcount();
        //useful at the end when the bytes  < MAXBUFFER are read.
        size_t bytes_sent = 0;

        //Execs until all the read bytes are sent.
        while(bytes_sent < bytes_read) {
          size_t bytes_sent_now = send(client_fd, data_buffer + bytes_sent, bytes_read - bytes_sent, 0);  
          if(bytes_sent_now < 0) {
            std::cerr << "Error sending data" << std::endl;
            file_data.close();
            return;
          }
          bytes_sent += bytes_sent_now;
        }
      }
      file_data.close();
      cout<<"File Transfer Complete!"<<endl;
}


void HandleGet(string recvd_data, int client_fd) {
    if(recvd_data.find("GET / HTTP") != string::npos) {
      send(client_fd, ack.c_str(), ack.size(),0);
    } else if(recvd_data.find("GET /echo/") != string :: npos) {
      int ending_ind = recvd_data.find("HTTP/1.1")-1;
      string fixed = "GET /echo/";
      string return_string = recvd_data.substr(fixed.size(),ending_ind - fixed.size());
      string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
                       + to_string(return_string.size()) + "\r\n\r\n" + return_string;
      ssize_t bytes_sent = 0;
      //incase echo contains some enormous value.
      while (bytes_sent < response.size()) {
          ssize_t sent_now = send(client_fd, response.c_str() + bytes_sent, response.size() - bytes_sent, 0);
          if (sent_now < 0) {
              perror("Error sending response");
              break;
          }
          bytes_sent += sent_now;
      }
    } else if(recvd_data.find("GET /user-agent HTTP") != string::npos) {
      // HTTP added to avoid responding to /user-agentUser-Agent:
      string target = "User-Agent:";
      if(recvd_data.find(target) == string :: npos) {
        send(client_fd, nack_bad_request.c_str(), nack_bad_request.size(),0);
      }
      int idx = recvd_data.find(target) + target.size() + 1;
      string return_string = "";
      while(recvd_data[idx]!='\r') return_string.push_back(recvd_data[idx++]);
      string response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: "
                       + to_string(return_string.size()) + "\r\n\r\n" + return_string;
      send(client_fd, response.c_str(), response.size(), 0);
    } else if(recvd_data.find("GET /files/") != string::npos) {
      send_file_tcp(recvd_data, client_fd);
    } else {
      send(client_fd, nack_not_found.c_str(), nack_not_found.size(),0);
    }
}


void recv_file_tcp(string recvd_data, int client_fd) {
  string fixed = "/upload/";
  int start_ind = recvd_data.find(fixed) + fixed.length();
  int end_ind = recvd_data.find(" HTTP");
  string file_name = recvd_data.substr(start_ind, end_ind - start_ind);
  fixed = "Content-Length: ";
  start_ind = recvd_data.find(fixed);
  if(start_ind == string :: npos) {
    send(client_fd, nack_bad_request.c_str(), nack_bad_request.size(),0);
    return;
  }
  start_ind += fixed.length();
  string cl = "";
  while(recvd_data[start_ind]!='\r' and recvd_data[start_ind]!='\n') {
    cl.push_back(recvd_data[start_ind++]);
  }
  ssize_t content_length = stoull(cl);

  //File can be created by ofstream automatically.
  filesystem::create_directories("Uploaded");
  string file_path = "./Uploaded/"+file_name;
  ofstream file_data(file_path, ios::binary);

  size_t bytes_recvd = 0;

  char data_buffer[MAXBUFFER];
  cout<<"Original size : "<<content_length<<endl;
  while(bytes_recvd < content_length) {
    size_t bytes_recvd_now = recv(client_fd,data_buffer,MAXBUFFER,0);
    if(bytes_recvd_now < 0) {
      cerr<<"Error while recv"<<endl;
      break;
    }
    file_data.write(data_buffer,bytes_recvd_now);
    bytes_recvd += bytes_recvd_now;
  }
  cout<<" Bytes recvd : "<<bytes_recvd<<endl;
  if(bytes_recvd == content_length) {
    send(client_fd, ack.c_str(), ack.length(), 0);
  } else {
    send(client_fd, nack_bad_request.c_str(), nack_bad_request.length(), 0);
  }
  file_data.close();
}
void HandlePost(string recvd_data, int client_fd) {
  if(recvd_data.find("/upload") != string::npos) {
    cout<<"Testing Endpoint"<<endl;
    recv_file_tcp(recvd_data, client_fd);
  } else { 
    send(client_fd, nack_bad_request.c_str(), nack_not_found.size(), 0);
  }
}


void HandleConnection(int client_fd) {
    char recv_buf[MAXBUFFER];
    ssize_t bytes_recvd = recv(client_fd, recv_buf, MAXBUFFER, 0);
    if(bytes_recvd > 0) {
      string recvd_data(recv_buf, bytes_recvd);
      cout<<"CLIENT_"<<client_fd<<" REQUEST : "<<endl;
      cout<<recvd_data<<endl;
      if(recvd_data.find("GET") != string::npos) {
        HandleGet(recvd_data, client_fd);
      } else if(recvd_data.find("POST") != string :: npos) {
        HandlePost(recvd_data, client_fd);
      } else {
        send(client_fd, nack_bad_request.c_str(), nack_bad_request.size(),0);
      }
    } else {
      cerr<<"No Data recvd from client"<<endl;
    }
    close(client_fd);
}


int main(int argc, char **argv) {
  // Flush after every cout / cerr
  cout << unitbuf;
  cerr << unitbuf;
  
  // You can use print statements as follows for debugging, they'll be visible when running tests.
  cout << "Logs from your program will appear here!\n";

  // Uncomment this block to pass the first stage
  
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
   cerr << "Failed to create server socket\n";
   return 1;
  }
  
  // Since the tester restarts your program quite often, setting SO_REUSEADDR
  // ensures that we don't run into 'Address already in use' errors
  int reuse = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
    cerr << "setsockopt failed\n";
    return 1;
  }
  
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(4221);
  
  if (bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr)) != 0) {
    cerr << "Failed to bind to port 4221\n";
    return 1;
  }
  
  int connection_backlog = 5;
  if (listen(server_fd, connection_backlog) != 0) {
    cerr << "listen failed\n";
    return 1;
  }
  
  struct sockaddr_in client_addr;
  int client_addr_len = sizeof(client_addr);
  
  cout << "Waiting for a client to connect...\n";
  
  //Creating a Thread Pool 
  ThreadPool threadPool(8);

  while(1) {
    int client_fd = accept(server_fd, (struct sockaddr *) &client_addr, (socklen_t *) &client_addr_len);
    if (client_fd < 0) {
          cerr << "Client Connect Attempt Failed\n";
          continue;  // Go back to listening
      }
    cout << "CLIENT_"<<client_fd<<" CONNECTED"<<endl;
    threadPool.ExecuteTask(HandleConnection, client_fd);
  }
  close(server_fd);

  return 0;
}
