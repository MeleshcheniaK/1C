run_client:
	gcc -o client.build client.c
	./client.build 8888 127.0.0.1

run_server:
	gcc -o server.build server.c
	./server.build 8888 127.0.0.1

delete_build_cache:
	rm *.build config.txt
