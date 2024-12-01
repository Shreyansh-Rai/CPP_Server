### This is an implementation of a server in C++ that can connect simultaneously without crashing to multiple clients. It uses a Thread Pool implementation to do this. All of it built from scratch. 

### ./compile_and_run.sh to compile and run 
### Just dropping a couple example of what you can check out. you are ofcourse free to look at the server file to see what other endpoints are created!

#### (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 & (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 & (sleep 3 && printf "GET / HTTP/1.1\r\n\r\n") | nc localhost 4221 & wait

#### curl -O http://localhost:4221/files/4K_Video.mp4 

### Or more generally 

#### (sleep 3 && curl -O http://localhost:4221/files/4K_Video.mp4) &  (sleep 3 && curl -O http://localhost:4221/files/4K_Video.mp4) & wait

### It can tackle large files and parallel requests (To an extent ofcourse the ThreadPool is currently set to 8 threads. Might want to experiment with that to increase perf.)

### You can also post to the server. It only handles binary data so do not use multipart format. just throw in an octet stream and should be well handled. examples below : 

#### curl -X POST --data-binary "@./TestFiles/witcher.jpg" http://localhost:4221/upload/FilenameTemp3.jpg -H "Content-Type: application/octet-stream" & wait

#### curl -X POST --data-binary "@./TestFiles/4K_Video.mp4" http://localhost:4221/upload/FilenameTemp2.mp4 -H "Content-Type: application/octet-stream"

#### curl -X POST --data-binary "@./TestFiles/audio_test.mp3" http://localhost:4221/upload/FilenameTemp1.mp3 -H "Content-Type: application/octet-stream"

#### curl -X POST --data-binary "@./TestFiles/ziptest.zip" http://localhost:4221/upload/FilenameTemp4.zip -H "Content-Type: application/octet-stream"