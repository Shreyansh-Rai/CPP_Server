### This is an implementation of a server in C++ that can connect simultaneously without crashing to multiple clients. It uses a Thread Pool implementation to do this. All of it built from scratch. 

### Just dropping a couple example of what you can check out. you are ofcourse free to look at the server file to see what other endpoints are created!

#### (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 & (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 & (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 & wait

#### curl -O http://localhost:4221/files/4K_Video.mp4 

### Or more generally 

#### (sleep 3 && curl -O http://localhost:4221/files/4K_Video.mp4) &  (sleep 3 && curl -O http://localhost:4221/files/4K_Video.mp4) & wait

### It can tackle large files and parallel requests (To an extent ofcourse the ThreadPool is currently set to 8 threads. Might want to experiment with that to increase perf.)