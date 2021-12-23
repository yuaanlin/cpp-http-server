FROM silkeh/clang:latest
COPY . /usr/src/myapp
WORKDIR /usr/src/myapp
RUN clang++ -std=c++20 -pthread -o server server.cpp
EXPOSE 6167
CMD ["./server"]