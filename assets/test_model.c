#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <string.h>


// Hash data into a buffer specified by hash.
// The first 16 bytes of the buffer will be used for the hash.
// The 17th byte will be set to 0 (string terminator).
// This hash algorithm is stinky.
void HashArray(
		int t_count,
		double t_data[],
		char t_hash[]) {
	char* raw = (char*)t_data;
	int max = t_count * sizeof(double);
	memset(t_hash, 0, 17);
	for (int i = 0; i < max; i++) {
		t_hash[i%16] ^= raw[i];
	}
}


void FDumpArray(
		FILE* t_file,
		int t_count,
		double t_data[]) {
	for (int i = 0; i < t_count; i++) {
		fprintf(t_file, "%f\n", t_data[i]);
	}
}


// Read floating point values from a file, up to the number set in t_count.
// Returns the actual number of values read.
int FReadArray(
		FILE* t_file,
		int t_count,
		double t_data[]) {
	int value_count = 0;
	while (value_count < t_count) {
		if (fscanf(t_file, "%lf", &t_data[value_count]) == EOF)
			break;
		value_count++;
	}
	return value_count;
}


struct HttpResponse {
	int code_;
	int content_length_;
	char content_type_[0x100];
	char content_[0x1000];
};


struct HttpRequest {
	char method_[0x10];
	char endpoint_[0x100];
	char payload_[0x1000];
	char payload_type_[0x80];
	char host_[0x80];
	int port_;
};


void FormatHttpRequest(struct HttpRequest* t_request, char* t_buffer, int t_buflen) {
	// Write start line and some headers.
	memset(t_buffer, 0, t_buflen);
	int used = snprintf(
		t_buffer,
		t_buflen,
		"%s %s HTTP/1.1\nHost: %s:%d\nUser-Agent: bayes-bridge/0.1\nAccept: */*\n",
		t_request->method_,
		t_request->endpoint_,
		t_request->host_,
		t_request->port_);

	if (strstr(t_request->method_, "POST")) {
		// Add content headers and body.
		used += snprintf(
			t_buffer + used,
			t_buflen - used,
			"Content-Type: %s\nContent-Length: %lu\n\n%s",
			t_request->payload_type_,
			strlen(t_request->payload_),
			t_request->payload_);
	}
	else {
		// End metadata.
		used += snprintf(
			t_buffer + used,
			t_buflen - used,
			"\n");
	}
}


void ParseHttpResponse(struct HttpResponse* t_response, char* t_buffer) {
	// Initialize response structure.	
	memset(t_response->content_, 0, sizeof(t_response->content_));
	memset(t_response->content_type_, 0 , sizeof(t_response->content_type_));
	t_response->content_length_ = 0;
	t_response->code_ = 0;

	// Get response code from start line.
	char* rest = t_buffer;
	char* start_line = __strtok_r(rest, "\n", &rest);
	sscanf(start_line, "%*s%d", &t_response->code_);

	// Parse headers until empty line (end of metadata).
	char* header_line;
	while(!strcmp((header_line = __strtok_r(rest, "\n", &rest)), "")) {
		if (strstr(header_line, "Content-Length")) {
			sscanf(header_line, "%*s%d", &t_response->content_length_);
		}
		
		if (strstr(header_line, "Content-Type")) {
			sscanf(header_line, "%*s%s", t_response->content_type_);
		}
	}

	// Content.
	strncpy(t_response->content_, rest, 0xfff);
}


int HttpRequest(
		struct HttpRequest* t_request,
		struct HttpResponse* t_response) {
	// Create socket.
	int fd;
	if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("Unable to create socket.\n");
		return 404;
	}

	// Connect to ip over specified port.
	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(t_request->port_);
	if (inet_pton(AF_INET, t_request->host_, &address.sin_addr) <= 0) {
		printf("Unable to parse ip %s\n", t_request->host_);
		return 404;
	}
	if (connect(fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
		printf("Unable to connect to %s.\n", t_request->host_);
		return 404;
	}

	// Send HTTP request.
	char buffer[0x1000];
	FormatHttpRequest(t_request, buffer, sizeof(buffer));
	int sent = 0;
	int sent_total = 0;
	while ((sent = send(fd, buffer+sent_total, strlen(buffer+sent_total), 0)) > 0) {
		sent_total += sent;
	}

	// Receive HTTP response.
	int total = 0;
	int received = 0;
	while ((received = recv(fd, buffer + total, sizeof(buffer) - total, 0)) > 0 && (total += received) < sizeof(buffer)) { }
	ParseHttpResponse(t_response, buffer);

	// Close socket.
	close(fd);
	return t_response->code_;
}


void model_(int *CurSet,
		int *NoOfParams,
		int *NoOfDerived,
		int *TotalDataValues,
		int *MaxNoOfDataValues,
		int *NoOfDataCols,
		int *NoOfAbscissaCols,
		int *NoOfModelVectors,
		double	Params[],
		double	Derived[],
		double	Abscissa[],
		double	Signal[]) {

	// Command to run.
	char* command = "";

	// Initialize worker.
	struct HttpRequest request;
	struct HttpResponse response;
	int code;
	strcpy(request.method_, "POST");
	strcpy(request.host_, "127.0.0.1");
	strcpy(request.endpoint_, "/bayes_bridge_api/worker/init");
	strcpy(request.payload_type_, "application/json");
	strcpy(request.payload_, "{}");
	request.port_ = 5000;
	if ((code = HttpRequest(&request, &response)) != 200) {
		printf("Request to initialize worker failed with code %d:\n%s\n", code, response.content_);
		return;
	}

	// Write input and params to file.
	char worker_id[0x80];
	char input_filename[0x80];
	char param_filename[0x80];
	char output_filename[0x80];
	char derived_filename[0x80];
	sscanf(response.content_, "%s%s%s%s%s", worker_id, input_filename, param_filename, output_filename, derived_filename);
	FILE* input_file = fopen(input_filename, "w");
	if (input_file == NULL) {
		printf("Unable to create file %s for inputs.\n", input_filename);
		return;
	}
	FDumpArray(input_file, *NoOfAbscissaCols * *MaxNoOfDataValues, Abscissa);
	fclose(input_file);
	FILE* param_file = fopen(param_filename, "w");
	if (param_file == NULL) {
		printf("Unable to create file %s for params.\n", param_filename);
		return;
	}
	FDumpArray(param_file, *NoOfParams, Params);
	fclose(param_file);

	// Start worker.
	strcpy(request.endpoint_, "/bayes_bridge_api/worker/start");
	snprintf(request.payload_, sizeof(request.payload_), "{\"worker_id\":\"%s\", \"command\":\"%s\"}", worker_id, command);
	if ((code = HttpRequest(&request, &response)) != 200) {
		printf("Request to start worker failed with code %d:\n%s\n", code, response.content_);
		return;
	}

	// Wait for worker.
	strcpy(request.method_, "GET");
	snprintf(request.endpoint_, sizeof(request.endpoint_), "/bayes_bridge_api/worker/query?worker_id=%s", worker_id);
	strcpy(request.payload_type_, "");
	strcpy(request.payload_, "");
	do {
		HttpRequest(&request, &response);
		sleep(2);
	} while (!strstr(response.content_, "Done"));

	// Read derived and signal.
	FILE* output_file = fopen(output_filename, "r");
	if (output_file == NULL) {
		printf("Failed to open output file %s.", output_filename);
		return;
	}
	FReadArray(output_file, *TotalDataValues, Signal);
	fclose(output_file);
	FILE* derived_file = fopen(derived_filename, "r");
	if (derived_file == NULL) {
		printf("Failed to open derived file %s.", derived_filename);
		return;
	}
	FReadArray(derived_file, *NoOfDerived, Derived);
	fclose(derived_file);

	// Collect worker.
	strcpy(request.method_, "POST");
	strcpy(request.endpoint_, "/bayes_bridge_api/worker/collect");
	strcpy(request.payload_type_, "application/json");
	snprintf(request.payload_, sizeof(request.payload_), "{\"worker_id\":\"%s\"}", worker_id);
	if ((code = HttpRequest(&request, &response)) != 200) {
		printf("Request to collect worker failed with code %d:\n%s\n", code, response.content_);
	}

	// Clean up.
	remove(input_filename);
	remove(param_filename);
	remove(output_filename);
	remove(derived_filename);
	return;
}