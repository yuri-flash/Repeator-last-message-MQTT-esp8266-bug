all:
	gcc -g main.c -lpaho-mqtt3c  -I src -lpthread -o threads_mqtt
	
clean:
	rm threads_mqtt

teste:
	gcc -g teste.c -lpaho-mqtt3c  -I src -lpthread -o teste
