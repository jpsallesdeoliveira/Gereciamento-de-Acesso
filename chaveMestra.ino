// Leitura da porta serial
void SerialTask(void *pvParameters) {
    (void) pvParameters;
    for (;;) {  // Loop infinito da task
        if (Serial.available() > 0) {
            // Lê a linha recebida até o caractere de nova linha
            String option = Serial.readStringUntil('\n');
            // Exibe a mensagem recebida no monitor serial
            Serial.print("Recebido: ");
            Serial.println(option);
        }
        // Pausa a task por 100ms para evitar sobrecarga e permitir o agendamento de outras tasks
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Configurações iniciais
void setup() {
    // Inicializa a comunicação serial
    Serial.begin(115200);

    // Cria a task para leitura serial
    xTaskCreate(
        SerialTask,      // Função da task
        "SerialTask",    // Nome da task (para debug)
        4096,            // Tamanho da pilha (em bytes)
        NULL,            // Parâmetros da task (nenhum)
        1,               // Prioridade da task
        NULL             // Handle da task (não utilizado)
    );
}

// Laço principal
void loop() {
    
}