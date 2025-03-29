String event;

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
            // Processar opção recebida
            processMenu(option);
        }
        // Pausa a task por 100ms para evitar sobrecarga e permitir o agendamento de outras tasks
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// Mostrar menu
void showMenu() {
    Serial.println("Menu:");
    Serial.println("a. Cadastro do usuário");
    Serial.println("b. Listagem dos nomes dos usuários cadastrados");
    Serial.println("c. Listagem dos eventos");
    Serial.println("d. Liberação da porta 1");
    Serial.println("e. Liberação da porta 2");
    event = "menu";
}

// Adicionar usuário
void menuUser() {
    Serial.println();
    Serial.println("Cadastro do usuário:");
    Serial.println("i. Nome");
    Serial.println("ii. Senha");
    Serial.println("iii. Definição do tipo de usuário  (Administrador ou não).");
    Serial.println("iv. Menu principal");
    event = "user";
}

// Menu nome
void menuNome() {
    Serial.println();
    Serial.println("Digite o nome: ");
    event = "nome";
}

// Adicionar nome
void addNome(String nome) {
    event = "add_nome";
}

// Adicionar Senha
void addSenha(String senha) {
    event = "add_senha";
}

// Menu senha
void menuSenha() {
    Serial.println();
    Serial.println("Digite a senha: ");
    event = "senha";
}

// Menu tipo
void menuTipo() {
    Serial.println();
    Serial.println("Digite 1 para administrador ou 0 para não administrador: ");
    event = "tipo";
}

// Adicionar tipo
void addTipo(String option) {
    event = "addTipo";
}

// Listar nomes
void obterNomes() {
    Serial.println();
    Serial.println("Lista dos nomes...");
    Serial.println();
    showMenu();
    // Serial.println("i. Voltar para o menu principal");
    // event = "lista_nomes";
}

// Listar eventos
void obterEventos() {
    Serial.println();
    Serial.println("Lista dos eventos...");
    Serial.println();
    showMenu();
}

// Liberar porta 1
void liberar1() {
    Serial.println();
    Serial.println("Porta 1 liberada...");
    Serial.println();
    showMenu();
}

// Liberar porta 2
void liberar2() {
    Serial.println();
    Serial.println("Porta 2 liberada...");
    Serial.println();
    showMenu();
}

// Adicionar fila
void addFila(String nome) {

}

// Mostrar eventos
void showEvents() {
    Serial.println("Eventos...");
    showMenu();
}

// Processar opção
void processMenu(String option) {
    if (event == "menu") {
        if (option == "a") {
            menuUser();
        } else if (option == "b") {
            obterNomes();
        } else if (option == "c") {
            obterEventos();
        } else if (option == "d") {
            liberar1();
        } else if (option == "e") {
            liberar2();
        } else {
            Serial.println("Opção inválida.");
            Serial.println();
            showMenu();
        }
    } else if (event == "user") {
        if (option == "i") {
            menuNome();
        } else if (option == "ii") {
            menuSenha();
        } else if (option == "iii") {
            menuTipo();
        } else if (option == "iv") {
            showMenu();
        } else {
            Serial.println("Opção inválida.");
            menuUser();
        }
    } else if (event == "events") {

    } else if (event == "port1") {

    } else if (event == "port2") {

    } else if (event == "nome") {
        addFila(option);
        menuUser();
    } else if (event == "senha") {
        addSenha(option);
        menuUser();
    } else if (event == "tipo") {
        addTipo(option);
        menuUser();
    }
}

// Configurações iniciais
void setup() {
    // Inicializa a comunicação serial
    Serial.begin(115200);
    delay(100);

    // Mostrar menu
    showMenu();

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
