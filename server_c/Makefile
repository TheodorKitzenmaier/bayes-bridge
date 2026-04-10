all:
	g++ -c request_handler.cc -o request_handler.o
	g++ -c worker.cc -o worker.o
	g++ -c main.cc -o main.o
	g++ main.o worker.o request_handler.o -o bayes_bridge

clean:
	rm request_handler.o
	rm worker.o
	rm main.o
