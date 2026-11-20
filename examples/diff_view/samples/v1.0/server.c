/*
 * server.c - a small server of static files (version 1.0).
 * An example file for the diff view: the same file in version 1.1 is
 * in ../v1.1/server.c.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS 64
#define BUFFER_SIZE 4096
#define DEFAULT_PORT 8080

typedef struct client_t {
	int socket;
	char buffer[BUFFER_SIZE];
	size_t used;
} client_t;

static client_t clients[MAX_CLIENTS];
static int client_count = 0;

static const char* content_type(const char* path) {
	const char* dot = strrchr(path, '.');
	if (!dot) return "application/octet-stream";
	if (strcmp(dot, ".html") == 0) return "text/html";
	if (strcmp(dot, ".css") == 0) return "text/css";
	if (strcmp(dot, ".js") == 0) return "application/javascript";
	if (strcmp(dot, ".png") == 0) return "image/png";
	return "application/octet-stream";
}

static int add_client(int socket) {
	if (client_count >= MAX_CLIENTS) return -1;
	clients[client_count].socket = socket;
	clients[client_count].used = 0;
	return client_count++;
}

static void remove_client(int index) {
	clients[index] = clients[client_count - 1];
	client_count--;
}

static int read_request(client_t* client, char* path, size_t size) {
	char* line_end = strstr(client->buffer, "\r\n");
	char method[8];
	if (!line_end) return 0;
	*line_end = '\0';
	if (sscanf(client->buffer, "%7s %255s", method, path) != 2) return -1;
	if (strcmp(method, "GET") != 0) return -1;
	(void)size;
	return 1;
}

static void old_handler(client_t* client) {
	/* kept for the clients of version 0.9 */
	printf("old request on socket %d\n", client->socket);
}

static long file_size(FILE* file) {
	long size;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	return size;
}

static void send_file(client_t* client, const char* path) {
	FILE* file = fopen(path, "rb");
	long size;
	if (!file) {
		printf("404 %s\n", path);
		return;
	}
	size = file_size(file);
	printf("200 %s (%ld bytes, %s)\n", path, size, content_type(path));
	fclose(file);
	(void)client;
}

static void handle(client_t* client) {
	char path[256];
	int result = read_request(client, path, sizeof(path));
	if (result < 0) {
		old_handler(client);
		return;
	}
	if (result == 0) return;
	send_file(client, path[0] == '/' ? path + 1 : path);
}

static void print_usage(const char* name) {
	printf("usage: %s [port]\n", name);
	printf("  serves the files of the current folder\n");
}

static int parse_port(const char* text) {
	int port = atoi(text);
	if (port <= 0 || port > 65535) return DEFAULT_PORT;
	return port;
}

static void print_banner(int port) {
	printf("serving on port %d\n", port);
	printf("press Ctrl+C to stop\n");
}

static void serve(int port) {
	int i;
	print_banner(port);
	for (i = 0; i < client_count; i++) {
		handle(&clients[i]);
	}
}

static void shutdown_clients(void) {
	while (client_count > 0) {
		remove_client(client_count - 1);
	}
}

int main(int argc, char** argv) {
	int port = DEFAULT_PORT;
	if (argc > 2) {
		print_usage(argv[0]);
		return 1;
	}
	if (argc == 2) port = parse_port(argv[1]);
	add_client(0);
	serve(port);
	shutdown_clients();
	return 0;
}
