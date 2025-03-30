// BIBLIOTECAS
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

bool webStatus = false;

// Replace with your network credentials
const char* ssid = "Eloinet_2G_Nome";
const char* password = "14202411";

bool ledState = 0;
const int ledPin = 18;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
html {
    font-family: Arial, Helvetica, sans-serif;
    text-align: center;
}
h1 {
    font-size: 1.8rem;
    color: white;
}
h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
}
.topnav {
    overflow: hidden;
    background-color: #143642;
}
body {
    margin: 0;
}
.content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
}
.card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
}
.button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
    }
    /*.button:hover {background-color: #0f8b8d}*/
    .button:active {
    background-color: #0f8b8d;
    box-shadow: 2 2px #CDCDCD;
    transform: translateY(2px);
    }
    .state {
    font-size: 1.5rem;
    color:#8c8c8c;
    font-weight: bold;
    }
</style>
<title>ESP Web Server</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
<div class="topnav">
    <h1>ESP WebSocket Server</h1>
</div>
<div class="content">
    <div class="card">
    <h2>Output - GPIO 2</h2>
    <p class="state">state: <span id="state">%STATE%</span></p>
    <p><button id="button" class="button">Toggle</button></p>
    </div>
</div>
<script>
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);
function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
}
function onOpen(event) {
    console.log('Connection opened');
}
function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
}
function onMessage(event) {
    var state;
    if (event.data == "1"){
    state = "ON";
    }
    else{
    state = "OFF";
    }
    document.getElementById('state').innerHTML = state;
}
function onLoad(event) {
    initWebSocket();
    initButton();
}
function initButton() {
    document.getElementById('button').addEventListener('click', toggle);
}
function toggle(){
    websocket.send('toggle');
}
</script>
</body>
</html>
)rawliteral";

void notifyClients() {
ws.textAll(String(ledState));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
AwsFrameInfo *info = (AwsFrameInfo*)arg;
if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle") == 0) {
    ledState = !ledState;
    notifyClients();
    }
}
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
            void *arg, uint8_t *data, size_t len) {
    switch (type) {
    case WS_EVT_CONNECT:
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        break;
    case WS_EVT_DISCONNECT:
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
        break;
    case WS_EVT_DATA:
        handleWebSocketMessage(arg, data, len);
        break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
        break;
}
}

void initWebSocket() {
ws.onEvent(onEvent);
server.addHandler(&ws);
}

String processor(const String& var){
Serial.println(var);
webStatus = true;
if(var == "STATE"){
    if (ledState){
    return "ON";
    }
    else{
    return "OFF";
    }
}
return String();
}

//FS
String fsStatus;
String users;
String events;
String user;
String porta;

//INICIAR FS
void initFs() {
    if(!SPIFFS.begin()){
        fsStatus = "0";
    } else {
        fsStatus = "1";
    }
    if (fsStatus == "1") {
        // usuários
        if (SPIFFS.exists("/users.txt")) {
            users = readFile("/users.txt");
        } else {
            writeFile("/users.txt", "null");
            users = readFile("/users.txt");
        }
        // eventos
        if (SPIFFS.exists("/events.txt")) {
            events = readFile("/events.txt");
        } else {
            writeFile("/events.txt", "null");
            events = readFile("/events.txt");
        }
    }
}

// ocultar senha
String converterParaAsteriscos(String texto) {
    String resultado = "";
    for (int i = 0; i < texto.length(); i++) {
      resultado += "*";
    }
    return resultado;
}

String event;

// Variáveis para controle do tempo e estado das portas
unsigned long tempoInicial1 = 0;
bool ledLigado1 = false;
unsigned long tempoInicial2 = 0;
bool ledLigado2 = false;

// Leitura da porta serial
void SerialTask(void *pvParameters) {
    (void) pvParameters;
    for (;;) {  // Loop infinito da task
        if (Serial.available() > 0) {
            // Lê a linha recebida até o caractere de nova linha
            String option = Serial.readStringUntil('\n');
            // Exibe a mensagem recebida no monitor serial
            Serial.print("Recebido: ");
            if (event == "login_senha") {
                Serial.println(converterParaAsteriscos(option));
            } else {
                Serial.println(option);
            }
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
    users = addUser(users, nome, "|", "=");
    writeFile("/users.txt", users.c_str());
    users = readFile("/users.txt");
    Serial.println("Users: " + users);
    event = "add_nome";
    user = nome;
}

// Adicionar Senha
void addSenha(String senha) {
    users = addPassword(users, user, senha, "|", "=");
    writeFile("/users.txt", users.c_str());
    users = readFile("/users.txt");
    Serial.println("Users: " + users);
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
    users = addUserType(users, user, option, "|", "=");
    writeFile("/users.txt", users.c_str());
    users = readFile("/users.txt");
    Serial.println("Users: " + users);
    event = "addTipo";
}

// Listar nomes
void obterNomes() {
    Serial.println();
    Serial.println("Lista dos usuários:");
    Serial.println(listUsers(users, "|", "="));
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
    digitalWrite(18, HIGH);
    ledLigado1 = true;
    tempoInicial1 = millis();
    Serial.println("Porta 1 liberada...");
    Serial.println();
    showMenu();
}

// Liberar porta 2
void liberar2() {
    Serial.println();
    digitalWrite(19, HIGH);
    ledLigado2 = true;
    tempoInicial2 = millis();
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

// Logar
void login() {
    Serial.println();
    Serial.println("Digite o nome do usuario: ");
    event = "login";
}

// Processar opção
void processMenu(String option) {
    if (event == "menu") {
        if (option == "a") {
            menuUser();
        } else if (option == "b") {
            obterNomes();
        } else if (option == "c") {
            Serial.println();
            Serial.println("Eventos:");
            Serial.println(listEvents(events, "|"));
            Serial.println();
            showMenu();
        } else if (option == "d") {
            webStatus = false;
            Serial.println();
            Serial.println("Liberar porta 1: ");
            // liberar1();
            login();
            porta = "1";
        } else if (option == "e") {
            webStatus = false;
            Serial.println();
            Serial.println("Liberar porta 2: ");
            login();
            porta = "2";
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
            Serial.println();
            showMenu();
        } else {
            Serial.println("Opção inválida.");
            menuUser();
        }
    } else if (event == "events") {

    } else if (event == "port1") {

    } else if (event == "port2") {

    } else if (event == "nome") {
        addNome(option);
        menuUser();
    } else if (event == "senha") {
        addSenha(option);
        menuUser();
    } else if (event == "tipo") {
        addTipo(option);
        menuUser();
    } else if (event == "login") {
        if (findUserItem__(users, option, "|", "=")) {
            Serial.println();
            Serial.println("Digite a senha:");
            event = "login_senha";
            user = option;
        } else {
            Serial.println();
            Serial.println("Usuário inválido");
            Serial.println();
            showMenu();
        }
    } else if (event == "login_senha") {
        if (getUserPassword(users, user, "|", "=") == option) {
            if (porta == "1") {
                tempoInicial1 = millis();
                ledLigado1 = true;
                liberar1();
                events = addQueue(events, user + " abriu a porta 1", "|");
                writeFile("/events.txt", events.c_str());
                events = readFile("/events.txt");
            }
            if (porta == "2") {
                tempoInicial2 = millis();
                ledLigado2 = true;
                liberar2();
                events = addQueue(events, user + " abriu a porta 2", "|");
                writeFile("/events.txt", events.c_str());
                events = readFile("/events.txt");
            }
        } else {
            Serial.println();
            Serial.println("Senha inválida");
            Serial.println();
            showMenu();
        }
    }
}

// LISTAR EVENTOS
String listEvents(String eventsQueue, String eventSeparator) {
    // Verifica se a fila está vazia ou contém "null"
    if (calculateMessages(eventsQueue, eventSeparator) <= 0 || eventsQueue == "null") {
        return "";
    }
    
    String lista = "";
    int inicio = 0;
    int posEvent;
    
    // Percorre cada evento na fila utilizando o separador de eventos
    while ((posEvent = eventsQueue.indexOf(eventSeparator, inicio)) != -1) {
        String eventItem = eventsQueue.substring(inicio, posEvent);
        lista += eventItem + "\n";  // Adiciona o evento e quebra de linha
        inicio = posEvent + eventSeparator.length();
    }
    
    // Adiciona o último evento (após o último separador)
    lista += eventsQueue.substring(inicio);
    
    return lista;
}

//CALCULAR MENSAGENS
int calculateMessages(String messageQueue, String separator_) {
    int count__ = 0;
    for (int i = 0; i <= messageQueue.length(); i = i + 1) {
        if (messageQueue.substring(i, i + 1) == separator_) {
            count__ = count__ + 1;
        }
    }
    if (messageQueue == "null") {
        return 0;
    } else if (count__ == 0) {
        return 1;
    } else {
        return count__ + 1;
    }
}

// PESQUISAR USUÁRIO NA FILA
bool findUserItem__(String credentialsQueue, String username, String itemSeparator, String keyValueSeparator) {
    // Verifica se a fila está vazia ou contém "null"
    if (calculateMessages(credentialsQueue, itemSeparator) <= 0 || credentialsQueue == "null") {
        return false;
    }
    
    int inicio = 0;
    int posItem;
    
    // Percorre cada item da fila usando o separador de itens
    while ((posItem = credentialsQueue.indexOf(itemSeparator, inicio)) != -1) {
        String item = credentialsQueue.substring(inicio, posItem);
        
        // Verifica se o item não está vazio
        if (item.length() > 0) {
            int posSeparator = item.indexOf(keyValueSeparator);
            String userPart;
            // Se encontrar o separador, extrai a parte do usuário
            if (posSeparator != -1) {
                userPart = item.substring(0, posSeparator);
            } else {
                // Caso não haja separador, o item contém apenas o usuário
                userPart = item;
            }
            // Se o usuário procurado for encontrado, retorna o item completo
            if (userPart == username) {
                return true;
            }
        }
        inicio = posItem + itemSeparator.length();
    }
    
    // Verifica o último item (após o último separador)
    String item = credentialsQueue.substring(inicio);
    if (item.length() > 0) {
        int posSeparator = item.indexOf(keyValueSeparator);
        String userPart;
        if (posSeparator != -1) {
            userPart = item.substring(0, posSeparator);
        } else {
            userPart = item;
        }
        if (userPart == username) {
            return true;
        }
    }
    
    // Caso o usuário não seja encontrado, retorna string vazia
    return false;
}

// PESQUISAR USUÁRIO NA FILA
String findUserItem(String credentialsQueue, String username, String itemSeparator, String keyValueSeparator) {
    // Verifica se a fila está vazia ou contém "null"
    if (calculateMessages(credentialsQueue, itemSeparator) <= 0 || credentialsQueue == "null") {
        return "";
    }
    
    int inicio = 0;
    int posItem;
    
    // Percorre cada item da fila usando o separador de itens
    while ((posItem = credentialsQueue.indexOf(itemSeparator, inicio)) != -1) {
        String item = credentialsQueue.substring(inicio, posItem);
        
        // Verifica se o item não está vazio
        if (item.length() > 0) {
            int posSeparator = item.indexOf(keyValueSeparator);
            String userPart;
            // Se encontrar o separador, extrai a parte do usuário
            if (posSeparator != -1) {
                userPart = item.substring(0, posSeparator);
            } else {
                // Caso não haja separador, o item contém apenas o usuário
                userPart = item;
            }
            // Se o usuário procurado for encontrado, retorna o item completo
            if (userPart == username) {
                return item;
            }
        }
        inicio = posItem + itemSeparator.length();
    }
    
    // Verifica o último item (após o último separador)
    String item = credentialsQueue.substring(inicio);
    if (item.length() > 0) {
        int posSeparator = item.indexOf(keyValueSeparator);
        String userPart;
        if (posSeparator != -1) {
            userPart = item.substring(0, posSeparator);
        } else {
            userPart = item;
        }
        if (userPart == username) {
            return item;
        }
    }
    
    // Caso o usuário não seja encontrado, retorna string vazia
    return "";
}


// RETORNAR SENHA DO USUÁRIO
String getUserPassword(String credentialsQueue, String username, String itemSeparator, String keyValueSeparator) {
    // Verifica se a fila está vazia ou contém "null"
    if (calculateMessages(credentialsQueue, itemSeparator) <= 0 || credentialsQueue == "null") {
        return "";
    }
    
    int inicio = 0;
    int posItem;
    
    // Percorre cada item da fila
    while ((posItem = credentialsQueue.indexOf(itemSeparator, inicio)) != -1) {
        String item = credentialsQueue.substring(inicio, posItem);
        // Localiza o primeiro separador para extrair o nome de usuário
        int posFirst = item.indexOf(keyValueSeparator);
        if (posFirst != -1) {
            String userPart = item.substring(0, posFirst);
            if (userPart == username) {
                // Verifica se há segundo separador (formato "usuário=senha=tipo")
                int posSecond = item.indexOf(keyValueSeparator, posFirst + keyValueSeparator.length());
                if (posSecond != -1) {
                    // Retorna a parte que está entre o primeiro e o segundo separador (senha)
                    return item.substring(posFirst + keyValueSeparator.length(), posSecond);
                } else {
                    // Caso não haja segundo separador, retorna a parte após o primeiro separador
                    return item.substring(posFirst + keyValueSeparator.length());
                }
            }
        }
        inicio = posItem + itemSeparator.length();
    }
    
    // Processa o último item (após o último separador)
    String item = credentialsQueue.substring(inicio);
    if (item.length() > 0) {
        int posFirst = item.indexOf(keyValueSeparator);
        if (posFirst != -1) {
            String userPart = item.substring(0, posFirst);
            if (userPart == username) {
                int posSecond = item.indexOf(keyValueSeparator, posFirst + keyValueSeparator.length());
                if (posSecond != -1) {
                    return item.substring(posFirst + keyValueSeparator.length(), posSecond);
                } else {
                    return item.substring(posFirst + keyValueSeparator.length());
                }
            }
        }
    }
    
    // Se o usuário não for encontrado, retorna string vazia
    return "";
}

// ADICIONAR USUÁRIO
String addUser(String credentialsQueue, String username, String itemSeparator, String keyValueSeparator) {
    // Verifica se o usuário já existe (utilize a função findUserItem se existir)
    if (findUserItem(credentialsQueue, username, itemSeparator, keyValueSeparator) != "") {
        return credentialsQueue;  // usuário já existe; não faz alteração
    }
    
    // Cria novo item com senha vazia e tipo padrão "0"
    String newItem = username + keyValueSeparator + keyValueSeparator + "0";
    
    // Se a fila já tiver itens, adiciona com o separador; caso contrário, substitui
    if (calculateMessages(credentialsQueue, itemSeparator) > 0 && credentialsQueue != "null") {
        credentialsQueue = credentialsQueue + itemSeparator + newItem;
    } else {
        credentialsQueue = newItem;
    }
    
    return credentialsQueue;
}

// ATUALIZAR SENHA DO USUÁRIO
String addPassword(String credentialsQueue, String username, String newPassword, String itemSeparator, String keyValueSeparator) {
    String newQueue = "";
    int inicio = 0;
    int posItem;
    
    // Percorre cada item da fila
    while ((posItem = credentialsQueue.indexOf(itemSeparator, inicio)) != -1) {
        String item = credentialsQueue.substring(inicio, posItem);
        int posFirst = item.indexOf(keyValueSeparator);
        if (posFirst != -1) {
            String userPart = item.substring(0, posFirst);
            // Se o usuário for encontrado, atualiza a senha mantendo o tipo
            if (userPart == username) {
                int posSecond = item.indexOf(keyValueSeparator, posFirst + keyValueSeparator.length());
                String typePart = "0";  // valor padrão se não existir
                if (posSecond != -1) {
                    typePart = item.substring(posSecond + keyValueSeparator.length());
                }
                item = username + keyValueSeparator + newPassword + keyValueSeparator + typePart;
            }
        }
        // Concatena o item à nova fila
        if (newQueue == "") {
            newQueue = item;
        } else {
            newQueue = newQueue + itemSeparator + item;
        }
        inicio = posItem + itemSeparator.length();
    }
    
    // Processa o último item (após o último separador)
    String item = credentialsQueue.substring(inicio);
    if (item.length() > 0) {
        int posFirst = item.indexOf(keyValueSeparator);
        if (posFirst != -1) {
            String userPart = item.substring(0, posFirst);
            if (userPart == username) {
                int posSecond = item.indexOf(keyValueSeparator, posFirst + keyValueSeparator.length());
                String typePart = "0";
                if (posSecond != -1) {
                    typePart = item.substring(posSecond + keyValueSeparator.length());
                }
                item = username + keyValueSeparator + newPassword + keyValueSeparator + typePart;
            }
        }
        if (newQueue == "") {
            newQueue = item;
        } else {
            newQueue = newQueue + itemSeparator + item;
        }
    }
    
    return newQueue;
}

// ATUALIZAR TIPO DO USUÁRIO
String addUserType(String credentialsQueue, String username, String newType, String itemSeparator, String keyValueSeparator) {
    String newQueue = "";
    int inicio = 0;
    int posItem;
    
    // Percorre cada item da fila
    while ((posItem = credentialsQueue.indexOf(itemSeparator, inicio)) != -1) {
        String item = credentialsQueue.substring(inicio, posItem);
        int posFirst = item.indexOf(keyValueSeparator);
        if (posFirst != -1) {
            String userPart = item.substring(0, posFirst);
            // Se o usuário for encontrado, atualiza o tipo mantendo a senha atual
            if (userPart == username) {
                int posSecond = item.indexOf(keyValueSeparator, posFirst + keyValueSeparator.length());
                String passwordPart = "";
                if (posSecond != -1) {
                    passwordPart = item.substring(posFirst + keyValueSeparator.length(), posSecond);
                }
                item = username + keyValueSeparator + passwordPart + keyValueSeparator + newType;
            }
        }
        // Concatena o item à nova fila
        if (newQueue == "") {
            newQueue = item;
        } else {
            newQueue = newQueue + itemSeparator + item;
        }
        inicio = posItem + itemSeparator.length();
    }
    
    // Processa o último item (após o último separador)
    String item = credentialsQueue.substring(inicio);
    if (item.length() > 0) {
        int posFirst = item.indexOf(keyValueSeparator);
        if (posFirst != -1) {
            String userPart = item.substring(0, posFirst);
            if (userPart == username) {
                int posSecond = item.indexOf(keyValueSeparator, posFirst + keyValueSeparator.length());
                String passwordPart = "";
                if (posSecond != -1) {
                    passwordPart = item.substring(posFirst + keyValueSeparator.length(), posSecond);
                }
                item = username + keyValueSeparator + passwordPart + keyValueSeparator + newType;
            }
        }
        if (newQueue == "") {
            newQueue = item;
        } else {
            newQueue = newQueue + itemSeparator + item;
        }
    }
    
    return newQueue;
}

// LISTAR NOMES DOS USUÁRIOS
String listUsers(String credentialsQueue, String itemSeparator, String keyValueSeparator) {
    // Verifica se a fila está vazia ou contém "null"
    if (calculateMessages(credentialsQueue, itemSeparator) <= 0 || credentialsQueue == "null") {
        return "";
    }
    
    String lista = "";
    int inicio = 0;
    int posItem;
    
    // Percorre cada item da fila utilizando o separador de itens
    while ((posItem = credentialsQueue.indexOf(itemSeparator, inicio)) != -1) {
        String item = credentialsQueue.substring(inicio, posItem);
        // Extrai o nome do usuário (parte anterior ao primeiro separador "=")
        int posSeparator = item.indexOf(keyValueSeparator);
        String userPart = (posSeparator != -1) ? item.substring(0, posSeparator) : item;
        lista += userPart + "\n";
        inicio = posItem + itemSeparator.length();
    }
    
    // Processa o último item (após o último separador)
    String item = credentialsQueue.substring(inicio);
    if (item.length() > 0) {
        int posSeparator = item.indexOf(keyValueSeparator);
        String userPart = (posSeparator != -1) ? item.substring(0, posSeparator) : item;
        lista += userPart;
    }
    
    return lista;
}


//ADICIONAR FILA
String addQueue(String messageQueue, String message, String separator_) {
    if (calculateMessages(messageQueue, separator_) > 0) {
        messageQueue = messageQueue + separator_ + message;
        return messageQueue;
    } else {
        messageQueue = message;
        return messageQueue;
    }
}

//LER FILA
String readQueue(String messageQueue, String separator_) {
    int sizeMessage;
    sizeMessage = calculateMessages(messageQueue, separator_);
    if (sizeMessage > 0) {
        if (sizeMessage == 1) {
            return messageQueue;
        }
        for (int i = 0; i <= messageQueue.length(); i = i + 1) {
            if (messageQueue.substring(i, i + 1) == separator_) {
                return messageQueue.substring(0, i);
            }
        }
    } else {
        return messageQueue;
    }
    //RETORNO ADICIONADO
    return messageQueue;
}

//REMOVER FILA
String removeQueue(String messageQueue, String separator_) {
    int sizeMessage;
    sizeMessage = calculateMessages(messageQueue, separator_);
    if (sizeMessage > 0) {
        if (sizeMessage == 1) {
            messageQueue = "null";
            return messageQueue;
        }
        for (int i = 0; i <= messageQueue.length(); i++) {
            if (messageQueue.substring(i, i + 1) == separator_) {
                messageQueue = messageQueue.substring(i + 1, messageQueue.length());
                return messageQueue;
            }
        }
    } else {
        return messageQueue;
    }    
    //RETORNO ADICIONADO
    return messageQueue;
}

//LER ARQUIVO
String readFile(const char * path){
    File file = SPIFFS.open(path, "r");
    if (!file) {
        return String();
    }
    String value = "";
    while (file.available()) {
        char c = file.read();
        value = value + c;
    }
    file.close();
    return value;  
}

//ESCREVER ARQUIVO
bool writeFile(const char * path, const char * message) {
    File file = SPIFFS.open(path, "w");
    if (!file) {
        return false;
    }
    int bytesWritten = file.print(message);
    if (bytesWritten == 0) {
        file.close();
        return false;
    } else {
        file.close();
        return true;
    }
}

// Configurações iniciais
void setup() {
    // Inicializa a comunicação serial
    Serial.begin(115200);
    delay(100);

    // iniciar fs
    initFs();

    // portas
    pinMode(18, OUTPUT);
    pinMode(19, OUTPUT);

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

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi..");
    }

    // Print ESP Local IP Address
    Serial.println(WiFi.localIP());

    initWebSocket();

    // Route for root / web page
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html, processor);
    });

    // Start server
    server.begin();
}

// Laço principal
void loop() {
    // fechamento automático porta 1
    if (ledLigado1 && (millis() - tempoInicial1 >= 5000)) {
        digitalWrite(18, LOW);
        ledLigado1 = false;
        Serial.println();
        Serial.println("Porta 1 fechada");
        Serial.println();
        showMenu();
    }

    // fechamento automático porta 2
    if (ledLigado2 && (millis() - tempoInicial2 >= 5000)) {
        digitalWrite(19, LOW);
        ledLigado2 = false;
        Serial.println();
        Serial.println("Porta 2 fechada");
        Serial.println();
        showMenu();
    }

    ws.cleanupClients();
    if (webStatus) {
        digitalWrite(ledPin, ledState);
    }
}
