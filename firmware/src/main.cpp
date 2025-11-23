/*
Engenharia de Computação - UPF
Comunicação de dados em Aplicações Embarcadas
Marcelo Trindade Rebonatto
28/10/2023
Server Inicial BLE
Adaptado de Based on Neil Kolban example for IDF: 
    https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
    updates by chegewara
*/

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <vector>  
#include <string>  
#include <stdlib.h> // Required for atof()

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

// UUID do serviço:
#define SERVICE_UUID                "ce3ae69e-2292-401d-b314-910faef61474"

// Características de ESCRITA/LEITURA
#define CHAR_UUID_ADD_NOTA          "5efe17dd-e072-4604-a3d2-854e9430bb0a" // Escrever nota (float)
#define CHAR_UUID_REMOVE_NOTA       "ff03cbf2-c50d-4fe1-a0ff-7714fedac85a" // Escrever índice para remover
#define CHAR_UUID_LISTAR_NOTAS      "d11af84b-0693-428c-bf65-cf7380c45dde" // Ler todas as notas
#define CHAR_UUID_NOTA_POR_INDICE   "7a9f3efa-b186-4be7-9cff-d8b4dd460679" // Ler nota específica
        

// Características de NOTIFICAÇÃO
#define CHAR_UUID_COUNT_NOTIFY  "dfbfc486-d01c-4b77-8db0-26993e0eec0e" // Notifica qtd de notas
#define CHAR_UUID_MEDIA_STATUS      "b3fd6848-6bdd-46b2-b976-636465d8b072" // Notifica situação da média

// Variável que vai armazenar as notas
std::vector<std::string> listaDeNotas;

/* Global */
BLEDescriptor notifyDescriptor(BLEUUID((uint16_t)0x2902));

// um para cada
// BLEDescriptor notifyDescriptorCount(BLEUUID((uint16_t)0x2902));
// BLEDescriptor notifyDescriptorMedia(BLEUUID((uint16_t)0x2902));
// BLEDescriptor notifyDescriptorIndice(BLEUUID((uint16_t)0x2902));

// Ponteiros para as características (para acesso global)
BLECharacteristic *pCharac_add_nota;
BLECharacteristic *pCharac_remove_nota;
BLECharacteristic *pCharac_listar_notas;
BLECharacteristic *pCharac_nota_por_indice;
BLECharacteristic *pCharac_count_notify;
BLECharacteristic *pCharac_media_status;

// Variáveis para armazenar a soma das notas e a média
double soma_notas = 0;
double media_notas = 0;

// Tratamento do Anuncio de serviço quando disconecta 
class MyServerCallbacks: public BLEServerCallbacks {
    void onDisconnect(BLEServer* pServer) {
        Serial.println("Conexão desfeita ...\nVoltando a anunciar ...");
        BLEDevice::startAdvertising();
    };
};


// Função para atualizar as notificações 
void atualizarNotificacoes() {
    // 1 - Atualiza a contagem
    std::string MsgNumNotas = "Total de notas: " + std::to_string(listaDeNotas.size());
    pCharac_count_notify->setValue(MsgNumNotas);
    pCharac_count_notify->notify();
    Serial.println((String)"[Notify] Numero de notas enviado: " + MsgNumNotas.c_str());

    // 2 - Atualiza Média e Status
    if (listaDeNotas.size() > 0) {
        media_notas = soma_notas / listaDeNotas.size();
    }
    else {
        media_notas = 0.0;
    }

    String status = "";
    if (listaDeNotas.size() == 0) {
        status = "Sem notas";
    }
    else if (media_notas < 3.0) {
        status = "Reprovado";
    }
    else if (media_notas < 7.0) {
        status = "Exame";
    }
    else {
        status = "Aprovado";
    }

    // Formata a mensagem: "Media: 7.5 - Status: "
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "Media: %.2f - %s", media_notas, status.c_str());
    
    pCharac_media_status->setValue(std::string(buffer));
    pCharac_media_status->notify();
    Serial.println((String)"[Notify] Status enviado: " + buffer);
}

// Função para atualizar a lista de leitura 
void atualizarListaLeitura() {
    std::string todasAsNotas = "";
    if (listaDeNotas.size() > 0) {
        for(int i = 0; i < listaDeNotas.size(); i++) {
            todasAsNotas += listaDeNotas[i];
            todasAsNotas += ";"; 
        }
    } else {
        todasAsNotas = "Nenhuma nota salva";
    }
    pCharac_listar_notas->setValue(todasAsNotas);
}


// =======================================================================
// Tratamento de valores das Carateristicas 
// =======================================================================
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic){
        // Pega o valor escrito pelo cliente
        std::string value = pCharacteristic->getValue();
        // Pega o UUID da característica que foi escrita
        String uuid = pCharacteristic->getUUID().toString().c_str();

        // Converte os UUIDs definidos para String para facilitar a comparação, não é extritamente necessário
        String uuid_add_nota = CHAR_UUID_ADD_NOTA;
        String uuid_remove_nota = CHAR_UUID_REMOVE_NOTA;
        String uuid_nota_indice = CHAR_UUID_NOTA_POR_INDICE;

        // ===============================================================
        // 1- Se cliente escreveu na característica "Adicionar Nota" 
        // ===============================================================
        if (uuid.equals(uuid_add_nota)){
            if (value.length() > 0){
                Serial.print("--- Adicionar nota: ");
                Serial.println(value.c_str());

                // ===========================================
                // Adiciona a nota no vetor de notas
                listaDeNotas.push_back(value);
                // Adiciona a nota à variável (e converte de ASCII para Float)
                soma_notas += atof(value.c_str());

                atualizarListaLeitura();
                atualizarNotificacoes(); // Dispara os dois Notifies (Count e Média)
            }
        }

        // ===============================================================
        // 2- Se cliente exsccreveu na característica "Remover Nota"
        // ===============================================================
        else if (uuid.equals(uuid_remove_nota)) {
            if (value.length() > 0) {
                int indice = atoi(value.c_str());
                Serial.print("--- Remover nota indice: ");
                Serial.println(indice);

                if (indice >= 0 && indice < listaDeNotas.size()) {
                    // Remove do somatório antes de apagar
                    double valRemovido = atof(listaDeNotas[indice].c_str());
                    soma_notas -= valRemovido;
                    if(soma_notas < 0) soma_notas = 0; // Pra garantir que não vai ficar negativo

                    // Remove do vetor
                    listaDeNotas.erase(listaDeNotas.begin() + indice);
                    
                    Serial.println("Nota removida com sucesso.");
                    
                    atualizarListaLeitura();
                    atualizarNotificacoes(); // Dispara os dois Notifies
                }
                else {
                    Serial.println("Erro: Indice invalido para remocao.");
                }
            }
        }



        // ===============================================================
        // 3- Se cliente escreveu na característica "Nota por Índice"
        // ===============================================================
        else if (uuid.equals(uuid_nota_indice)) {
            if (value.length() > 0) {
                Serial.print("--- Consulta Indice: ");
                Serial.println(value.c_str());

                // Converte o valor revcebido para um índice
                // atoi (ASCII para inteiro) converte a string de C
                int indice = atoi(value.c_str());
                std::string notaEncontrada = "";

                // Vê se o índice é válido
                // Precisa ser maior ou igual a 0 e menor que o tamanho do vetor, senão é inválido
                if (indice >= 0 && indice < listaDeNotas.size()) {
                    // Pega a nota bem bonitinha
                    notaEncontrada = listaDeNotas[indice];  // 
                    Serial.print("Nota correspondente: ");
                    Serial.println(notaEncontrada.c_str());
                } 
                else {
                    // Considera indice inválido
                    notaEncontrada = "Erro: Indice invalido.";
                    Serial.println("Erro: Indice invalido.");
                }
                
                // Envia a resposta (nota encontrada ou não) de volta por notify
                pCharac_nota_por_indice->setValue(notaEncontrada);
                pCharac_nota_por_indice->notify();
           }
        }

    }
};


void setup() {
    delay(5000);
    Serial.begin(9600);
    BLEServer *pServer;                 /* Objeto para o Servidor BLE */
    BLEService *pService;               /* Objeto para o Servico */
    BLEAdvertising *pAdvertising;       /* Objeto para anuncio de Servidor */


    /* Servidor */
    Serial.println("Iniciando o Servidor BLE");
    BLEDevice::init("BLE Bloco de notas");      
    pServer = BLEDevice::createServer();

    /* Criando o Serviço */
    pService = pServer->createService(SERVICE_UUID);
    
    /* Criando e Ajustando as caracteristicas */

    // 1- Add nota (write)
    // 5efe17dd-e072-4604-a3d2-854e9430bb0a
    pCharac_add_nota = pService->createCharacteristic(
                          CHAR_UUID_ADD_NOTA,
                          BLECharacteristic::PROPERTY_WRITE 
                          );

    // 2- Remove nota (write)
    // ff03cbf2-c50d-4fe1-a0ff-7714fedac85a
    pCharac_remove_nota = pService->createCharacteristic(
                          CHAR_UUID_REMOVE_NOTA,
                          BLECharacteristic::PROPERTY_WRITE 
                          );

    // 3- Lista nota (read)
    // d11af84b-0693-428c-bf65-cf7380c45dde
    pCharac_listar_notas = pService->createCharacteristic(
                          CHAR_UUID_LISTAR_NOTAS,  
                          BLECharacteristic::PROPERTY_READ
                          );

    // 4- Pega nota por índice (write/notify) 
    // 7a9f3efa-b186-4be7-9cff-d8b4dd460679
    pCharac_nota_por_indice = pService->createCharacteristic(
                          CHAR_UUID_NOTA_POR_INDICE,  
                        //   BLECharacteristic::PROPERTY_READ  |
                          BLECharacteristic::PROPERTY_WRITE    | 
                          BLECharacteristic::PROPERTY_NOTIFY 
                          );

    // 5- Notifica numero de notas (Notify)
    // dfbfc486-d01c-4b77-8db0-26993e0eec0e
    pCharac_count_notify = pService->createCharacteristic(
                          CHAR_UUID_COUNT_NOTIFY,  
                          BLECharacteristic::PROPERTY_NOTIFY 
                          );

    // 6- Notify Media Status (Notify)
    // b3fd6848-6bdd-46b2-b976-636465d8b072
    pCharac_media_status = pService->createCharacteristic(
                          CHAR_UUID_MEDIA_STATUS,  
                          BLECharacteristic::PROPERTY_NOTIFY 
                          );

    // Descritores (para permitir notificação) 
    // O descritor 0x2902 é padrão para habilitar notificações
    pCharac_nota_por_indice->addDescriptor(&notifyDescriptor);
    // pCharac_count_notify->addDescriptor(&notifyDescriptor);
    pCharac_count_notify->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2902))); // Pode reusar ou criar um novo
    // pCharac_media_status->addDescriptor(&notifyDescriptor);
    pCharac_media_status->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2902))); // Pode reusar ou criar um novo


    /* Definido o valor inicial para a característica de "Listar" */
    pCharac_listar_notas->setValue("Nenhuma nota");
    // Valor inicial característica notifica média
    pCharac_media_status->setValue("Status: s/media");
    /* Definido o valor inicial para a característica de "Pegar nota pelo índice" */
    // pCharac_nota_por_indice->setValue("Nenhum índice informado");


    /* Para registrar os Callbacks */
    // Registra o callback do servidor (para desconexão)
    pServer->setCallbacks(new MyServerCallbacks());
    // Registra o callback de escrita para as características que recebem dados
    pCharac_add_nota->setCallbacks(new MyCharacteristicCallbacks());
    pCharac_remove_nota->setCallbacks(new MyCharacteristicCallbacks());
    pCharac_nota_por_indice->setCallbacks(new MyCharacteristicCallbacks());

    /* Iniciando o Serviço */
    pService->start();

    /* Criando e configurando o anuncio */ 
    pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x12);
    
    /* Inicia a anunciar */
    BLEDevice::startAdvertising();
    Serial.println("Anuncio configurado e iniciado. Pode-se conectar e ler do Cliente");
    
}

void loop() {
    delay(5000); // Delay pra abrir o monitor

}
