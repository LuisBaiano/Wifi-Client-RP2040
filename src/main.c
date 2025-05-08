#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "lwip/ip4_addr.h" // Para converter o IP de string para endereço
#include "lwip/tcp.h"


// --- Configuração ---
#define WIFI_SSID "R4WIFI"
#define WIFI_PASSWORD "12345678"
#define SERVER_IP_STR "192.168.4.1" // IP padrão do Access Point do Arduino/ESP
#define SERVER_PORT 80             // Porta padrão HTTP
#define PROXIMITY_SENSOR_PIN 24    // Pino GPIO do sensor de proximidade

#define POLL_INTERVAL_MS 500       // Intervalo entre verificações do sensor (milisegundos)
#define CONNECTION_TIMEOUT_MS 10000 // Tempo máximo para conectar ao WiFi (milisegundos)
#define HTTP_TIMEOUT_MS 5000       // Tempo máximo para a requisição HTTP (milisegundos)

/**
 * @brief Envia uma requisição HTTP GET simples para o servidor Arduino.
 *        Esta função é BLOQUEANTE. Ela tenta conectar, enviar a requisição
 *        e depois fecha a conexão.
 *
 * @param path O caminho da requisição (ex: "/ON" ou "/OFF").
 * @return true se a requisição foi enviada (não garante sucesso da recepção), false em caso de erro.
 */
bool send_simple_http_get(const char *path) {
    struct tcp_pcb *pcb = NULL; // Protocol Control Block para a conexão TCP
    err_t err = ERR_OK;
    ip_addr_t server_addr;

    printf("Tentando enviar GET para: %s\n", path);

    // Converte o IP do servidor de string para o formato do lwIP
    if (!ipaddr_aton(SERVER_IP_STR, &server_addr)) {
        printf("Erro: Endereço IP do servidor inválido: %s\n", SERVER_IP_STR);
        return false;
    }

    // --- 1. Cria um novo PCB TCP ---
    pcb = tcp_new();
    if (!pcb) {
        printf("Erro: Falha ao criar PCB TCP\n");
        return false;
    }

    // --- 2. Tenta conectar ao servidor ---
    cyw43_arch_lwip_begin();
    printf("Conectando a %s:%d...\n", SERVER_IP_STR, SERVER_PORT);
    // Inicia a tentativa de conexão. O último argumento (NULL) indica que não usaremos callback de conexão.
    err = tcp_connect(pcb, &server_addr, SERVER_PORT, NULL);
    cyw43_arch_lwip_end(); // Libera o stack

    if (err != ERR_OK) {
        printf("Erro: Falha ao iniciar conexão TCP (%d)\n", err);
        return false;
    }

    // --- 3. Espera pela Conexão (com timeout) ---
    uint32_t start_time = time_us_32();
    bool connected = false;
    printf("Aguardando conexão...\n");
    while (time_us_32() - start_time < HTTP_TIMEOUT_MS * 1000) {
         cyw43_arch_poll(); // Processa eventos de rede
         // Verifica o estado da conexão diretamente (simplificado)
         // Um estado diferente de CLOSED ou LISTEN geralmente indica progresso/conexão
         if (pcb->state != CLOSED && pcb->state != LISTEN && pcb->state != SYN_SENT) {
              connected = true;
              printf("Conectado!\n");
              break;
         }
         sleep_ms(10); // Pequena pausa para não sobrecarregar a CPU
    }

    if (!connected) {
         printf("Erro: Timeout ao conectar ao servidor TCP\n");
         cyw43_arch_lwip_begin();
         tcp_abort(pcb); // Aborta a tentativa de conexão
         cyw43_arch_lwip_end();
         return false;
    }


    // --- 4. Prepara e Envia a Requisição HTTP ---
    printf("Enviando requisição GET %s...\n", path);
    char request_buffer[200];
    int request_len = snprintf(request_buffer, sizeof(request_buffer),
                               "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
                               path, SERVER_IP_STR);

    if (request_len >= sizeof(request_buffer)) {
        printf("Erro: Buffer de requisição HTTP pequeno demais!\n");
        cyw43_arch_lwip_begin();
        tcp_close(pcb); // Tenta fechar normalmente
        cyw43_arch_lwip_end();
        return false;
    }

    // Envia os dados pela conexão TCP
    cyw43_arch_lwip_begin();
    err = tcp_write(pcb, request_buffer, request_len, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK) {
        // Tenta garantir que os dados sejam enviados imediatamente
        err = tcp_output(pcb);
    }
    cyw43_arch_lwip_end();

    if (err != ERR_OK) {
        printf("Erro: Falha ao enviar dados TCP (%d)\n", err);
        // Não precisa fechar aqui, o erro pode já ter fechado, ou fecharemos abaixo
    } else {
        printf("Requisição HTTP enviada.\n");
    }
    // Mesmo com erro no envio, tentamos fechar a conexão

    // --- 5. Fecha a Conexão ---
    // A requisição pede "Connection: close", então fechamos após enviar.
    printf("Fechando conexão TCP...\n");
    cyw43_arch_lwip_begin();
    err_t close_err = tcp_close(pcb);
    cyw43_arch_lwip_end();

    if (close_err != ERR_OK) {
        printf("Erro ao fechar TCP (%d), abortando.\n", close_err);
        cyw43_arch_lwip_begin();
        tcp_abort(pcb); // Força o fechamento se 'close' falhar
        cyw43_arch_lwip_end();
        return false; // Indica falha no fechamento
    }

    printf("Conexão fechada.\n");
    return true; // Requisição enviada e conexão fechada (ou abortada)
}

// --- Programa Principal ---
int main() {
    // Inicializa a comunicação serial e espera um pouco para o monitor serial conectar
    stdio_init_all();
    sleep_ms(2000); // Aumenta um pouco a espera inicial
    printf("========================================\n");
    printf("RP2040 W: Sensor Proximidade -> LED R4\n");
    printf("========================================\n");


    // Inicializa o chip WiFi
    if (cyw43_arch_init()) {
        printf("Erro: Falha ao inicializar WiFi\n");
        while(1); // Trava se o WiFi falhar
    }
    printf("Chip WiFi inicializado.\n");

    // Habilita o modo Station (para conectar a um Access Point)
    cyw43_arch_enable_sta_mode();
    printf("Modo WiFi Station habilitado.\n");

    // Conecta ao Access Point do Arduino R4 WiFi
    printf("Conectando ao WiFi AP: %s...\n", WIFI_SSID);
    // Tenta conectar por um tempo definido (timeout)
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, CONNECTION_TIMEOUT_MS)) {
        printf("Erro: Falha ao conectar ao WiFi AP '%s'. Verifique SSID e senha.\n", WIFI_SSID);
        while(1); // Trava se não conseguir conectar
    }
    printf("Conectado ao WiFi!\n");


    // Inicializa o pino GPIO para o sensor de proximidade
    gpio_init(PROXIMITY_SENSOR_PIN);
    gpio_set_dir(PROXIMITY_SENSOR_PIN, GPIO_IN); // Configura como entrada
    gpio_pull_down(PROXIMITY_SENSOR_PIN); // Habilita pull-down: lê LOW se nada conectado ou sem detecção
    printf("GPIO %d inicializado para Sensor de Proximidade (com Pull Down).\n", PROXIMITY_SENSOR_PIN);


    // Guarda o estado anterior do sensor para enviar requisição apenas na mudança
    bool estado_anterior_proximidade = false;
    // Lê o estado inicial
    estado_anterior_proximidade = gpio_get(PROXIMITY_SENSOR_PIN);
    printf("Estado inicial do sensor: %s\n", estado_anterior_proximidade ? "DETECTADO" : "NÃO DETECTADO");
    // Envia o estado inicial para o Arduino
    send_simple_http_get(estado_anterior_proximidade ? "/ON" : "/OFF");


    // Loop principal infinito
    while (true) {
        // Lê o estado atual do sensor
        bool estado_atual_proximidade = gpio_get(PROXIMITY_SENSOR_PIN);

        // Verifica se o estado do sensor mudou desde a última leitura
        if (estado_atual_proximidade != estado_anterior_proximidade) {
            printf("Mudança detectada! Estado atual: %s\n", estado_atual_proximidade ? "DETECTADO" : "NÃO DETECTADO");

            // Envia a requisição HTTP correspondente ao novo estado
            if (estado_atual_proximidade) {
                // Sensor detectou -> Envia comando para LIGAR o LED
                send_simple_http_get("/ON");
            } else {
                // Sensor não detectou -> Envia comando para DESLIGAR o LED
                send_simple_http_get("/OFF");
            }

            // Atualiza o estado anterior para a próxima comparação
            estado_anterior_proximidade = estado_atual_proximidade;

        } // Fim do if (mudança detectada)

        cyw43_arch_poll();

        // Pausa por um tempo definido antes da próxima verificação
        sleep_ms(POLL_INTERVAL_MS);

    } // Fim do while(true)

    // Código abaixo nunca será alcançado neste exemplo
    cyw43_arch_deinit();
    return 0;
}