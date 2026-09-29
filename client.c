#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int PORT;

// Thread para receber mensagens do servidor em tempo real
void *receive_messages(void *socket_desc) {
    int sock = *((int *)socket_desc);
    char buffer[1024];
    int bytes_read;

    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        // Limpa a linha atual e imprime a mensagem recebida para não quebrar a digitação
        printf("\r\33[2K%s", buffer); 
        printf("Você: ");
        fflush(stdout);
    }
    return NULL;
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024];
    char name[50];

    // Pergunta o nome de usuário logo no início
    printf("Digite o seu nome de usuário: ");
    fgets(name, sizeof(name), stdin);
    name[strcspn(name, "\n")] = 0;

    printf("Digite a porta do servidor: ");
    scanf("%d", &PORT);
    getchar(); // Limpa o \n do buffer

    printf("Digite o IP do servidor: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    inet_pton(AF_INET, buffer, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nErro ao conectar ao servidor.\n");
        return -1;
    }

    // Envia o nome de usuário imediatamente após conectar
    send(sock, name, strlen(name), 0);

    printf("[+] Conectado ao chat! Digite '/q' para sair.\n\n");

    pthread_t tid;
    pthread_create(&tid, NULL, receive_messages, &sock);
    pthread_detach(tid);

    while (1) {
        printf("Você: ");
        fgets(buffer, sizeof(buffer), stdin);
        
        if (strcasecmp(buffer, "/q\n") == 0) {
            break;
        }
        
        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}