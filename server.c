#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int PORT;
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

// Estrutura para armazenar informações do cliente
typedef struct {
    int socket;
    char name[50];
} Client;

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void write_log(const char *message) {
    FILE *log_file = fopen("server_log.txt", "a");
    if (log_file == NULL) {
        perror("Erro ao abrir o arquivo de log");
        return;
    }
    time_t now = time(NULL);
    char *timestamp = ctime(&now);
    timestamp[strlen(timestamp) - 1] = '\0'; // Remove a nova linha do final
    fprintf(log_file, "[%s] %s\n", timestamp, message);
    fclose(log_file);
}

// Adiciona o cliente à lista global
void add_client(int socket, char *name) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = socket;
            strncpy(clients[i].name, name, sizeof(clients[i].name) - 1);
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Remove o cliente da lista quando desconecta
void remove_client(int socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == socket) {
            clients[i].socket = 0;
            memset(clients[i].name, 0, sizeof(clients[i].name));
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Envia a mensagem para todos os clientes, exceto o remetente
void broadcast_message(char *message, int sender_socket) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0 && clients[i].socket != sender_socket) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Função executada por uma thread para cada cliente conectado
void *handle_client(void *arg) {
    int sock = *((int *)arg);
    free(arg);
    char buffer[BUFFER_SIZE];
    char formatted_message[BUFFER_SIZE + 60];
    char client_name[50] = "Anônimo";
    int bytes_read;

    // 1. O primeiro recv do cliente deve ser o seu nome
    bytes_read = recv(sock, client_name, sizeof(client_name) - 1, 0);
    if (bytes_read > 0) {
        client_name[bytes_read] = '\0';
        // Remove quebra de linha caso venha no nome
        client_name[strcspn(client_name, "\r\n")] = 0;
    }

    add_client(sock, client_name);
    printf("[+] %s entrou no chat.\n", client_name);

    char log_message[BUFFER_SIZE + 60];
    snprintf(log_message, sizeof(log_message), "%s entrou no chat.", client_name);
    write_log(log_message);

    // Notifica os outros que o usuário entrou
    snprintf(formatted_message, sizeof(formatted_message), "--- %s entrou no chat ---\n", client_name);
    broadcast_message(formatted_message, sock);

    // Loop de escuta de mensagens do cliente
    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        
        // Exibe no console do servidor
        printf("[%s]: %s", client_name, buffer);

        snprintf(log_message, sizeof(log_message), "[%s]: %s", client_name, buffer);
        if(log_message[strlen(log_message) - 1] != '\n') {
            strcat(log_message, "\n");
        }
        write_log(log_message);

        // Prepara a mensagem formatada para enviar aos outros clientes
        snprintf(formatted_message, sizeof(formatted_message), "[%s]: %s", client_name, buffer);
        broadcast_message(formatted_message, sock);
    }

    close(sock);
    remove_client(sock);
    printf("[-] %s desconectou.\n", client_name);

    snprintf(log_message, sizeof(log_message), "%s desconectou.", client_name);
    write_log(log_message);
    
    // Notifica os outros que o usuário saiu
    snprintf(formatted_message, sizeof(formatted_message), "--- %s saiu do chat ---\n", client_name);
    broadcast_message(formatted_message, sock);

    return NULL;
}

int main() {
    printf("Digite a porta do servidor: ");
    scanf("%d", &PORT);
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
        memset(clients[i].name, 0, sizeof(clients[i].name));
    }

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Erro ao criar socket");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Erro no bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Erro no listen");
        exit(EXIT_FAILURE);
    }

    printf("[+] Servidor rodando na porta %d...\n", PORT);

    while (1) {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) continue;

        int *pclient = malloc(sizeof(int));
        *pclient = new_socket;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, pclient);
        pthread_detach(tid);
    }

    return 0;
}