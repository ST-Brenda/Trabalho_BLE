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

// Serviço:
#define SERVICE_UUID       "ce3ae69e-2292-401d-b314-910faef61474"
// Características:
#define CHAR_UUID_ADD_NOTA "5efe17dd-e072-4604-a3d2-854e9430bb0a"
#define CHAR_UUID_LISTAR_NOTAS "d11af84b-0693-428c-bf65-cf7380c45dde"
#define CHAR_UUID_NOTA_POR_INDICE "7a9f3efa-b186-4be7-9cff-d8b4dd460679"    
#define CHAR_UUID_NOVA_NOTA_NOTIFY "dfbfc486-d01c-4b77-8db0-26993e0eec0e"               

// Antigos
// #define CHARACTERISTIC_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// #define CHARACTERISTIC_UUID2  "4f8fc790-743c-11ee-b962-0242ac120002"
// #define CHARACTERISTIC_UUID3  "cce9813b-77e9-4c65-9864-368f5d6ce66e"


// Variável que vai armazenar as notas
std::vector<std::string> listaDeNotas;

/* Global */
BLEDescriptor notifyDescriptor(BLEUUID((uint16_t)0x2902));

// int valor = 0;   // Não é mais usado  

// Ponteiros para as características (para acesso global)
BLECharacteristic *pCharac_add_nota;
BLECharacteristic *pCharac_listar_notas;
BLECharacteristic *pCharac_nota_por_indice;
BLECharacteristic *pCharac_nova_nota_notify;

// Ponteiro antigo para a característica
// BLECharacteristic *notifyCharacteristic; 

// Variáveis para armazenar a soma das notas e a média
double soma_notas = 0;
double media_notas = 0;


// void mostraAcesso(){
//     Serial.println("Acionado Write de característica específica");
// }



/* Tratamento do Anuncio de serviço quando disconecta */
class MyServerCallbacks: public BLEServerCallbacks {
    void onDisconnect(BLEServer* pServer) {
        Serial.println("Conexão desfeita ...\nVoltando a anunciar ...");
        BLEDevice::startAdvertising();
    };
};


/* tratamento de valores das Carateristicas */
class MyCharacteristicCallbacks: public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic){
        // Pega o valor escrito pelo cliente
        std::string value = pCharacteristic->getValue();
        // Pega o UUID da característica que foi escrita
        String uuid = pCharacteristic->getUUID().toString().c_str();

        // Converte os UUIDs definidos para String para facilitar a comparação, não é extritamente necessário
        String uuid_add_nota = CHAR_UUID_ADD_NOTA;
        String uuid_nota_indice = CHAR_UUID_NOTA_POR_INDICE;

        // ===============================================================
        // 1- Se cliente escreveu na característica "Adicionar Nota" 
        // ===============================================================
        if (uuid.equals(uuid_add_nota)){
            //mostraAcesso();  //Não usamos
            
            if (value.length() > 0){
                Serial.println("--- Adicionar nota ---");
                Serial.print("Nota recebida: ");
                Serial.println(value.c_str());

                // ===========================================
                // Adiciona a nota no vetor de notas
                listaDeNotas.push_back(value);
                // Converte value de string para float
                double nota_double = atof(value.c_str());
                // Adiciona a nota à variável
                soma_notas += nota_double;
                // Calcula a média das notas
                media_notas = soma_notas/listaDeNotas.size();
                //Mostra no monitor a média das notas
                Serial.println("\nMEDIA DE NOTAS:\n||||||");
                Serial.println(media_notas);
                Serial.println("||||||");
                

                // ===========================================
                // Atualiza a característica de "Listar Notas"
                // Pro cliente ler a lista atualizada
                std::string todasAsNotas = "";
                if (listaDeNotas.size() > 0) {
                    for(int i = 0; i < listaDeNotas.size(); i++) {
                        // todasAsNotas += std::to_string(i) + ": "; // Adiciona o índice
                        todasAsNotas += listaDeNotas[i];
                        todasAsNotas += ";"; // Separador de notas
                    }
                } 
                
                else {
                    todasAsNotas = "Nenhuma nota salva ainda";
                }
                // Define o novo valor para a característica de leitura
                // ARRUMAR - DEFINIR VALOR DE NOTIFY COMO:
                // "NOTA ADICIONADA - NUMERO DE NOTAS = x"
                // OU
                // "NOTA REMOVIDA - NUMERO DE NOTAS = x"

                // Nota adicionada:
                String notifica_nota_adicionada = "Nova nota! Total: ";
                notifica_nota_adicionada += listaDeNotas.size();

                // Nota removida:
                String notifica_nota_removida = "Nota removida! Numero de notas atual: ";
                notifica_nota_removida += listaDeNotas.size();


                // ARRUMAR - FAZER LÓGICA DE REMOVER NOTAS DEPOIS

                // Lista todas as notas
                pCharac_listar_notas->setValue(todasAsNotas);      
                Serial.println("----------------------------------------\nCaracterística 'ListarNotas' atualizada.\n----------------------------------------");

                // ==========================================================
                // 3. ENVIA NOTIFICAÇÃO DE "NOVA NOTA"
                // ==========================================================
                Serial.println("Notificação 'NovaNotaNotify' enviada!");
                Serial.println("-------------------------------------");
                Serial.println(notifica_nota_adicionada);
                Serial.println("-------------------------------------");
                pCharac_nova_nota_notify->setValue(notifica_nota_adicionada.c_str());
                pCharac_nova_nota_notify->notify();
                
            
            }
        }

        // ===============================================================
        // 2- Se cliente escreveu na característica "Nota por Índice"
        // ===============================================================
        else if (uuid.equals(uuid_nota_indice)) {
            if (value.length() > 0) {
                Serial.println("======= Pedido de Nota por Índice =======");
                Serial.print("Índice recebido: ");
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
                
                // Envia a resposta de volta por notify
                pCharac_nota_por_indice->setValue(notaEncontrada);
                pCharac_nota_por_indice->notify();
                Serial.println("Notificação 'NotaPorIndice' enviada com a resposta.");
                Serial.println("---------------------------------------------------");
            }
        }
        
        // Serial.print("UUID ");
        // Serial.println(uuid.c_str());
    }
};


void setup() {
    delay(5000);
    Serial.begin(9600);
    BLEServer *pServer;                 /* Objeto para o Servidor BLE */
    BLEService *pService;               /* Objeto para o Servico */


    // Não são mais usadas:
    // BLECharacteristic *pCharacteristic; /* Objeto para uma Caracteristica */
    // BLECharacteristic *pCharac;         /* Objeto para uma Caracteristica */
    
    BLEAdvertising *pAdvertising;       /* Objeto para anuncio de Servidor */


    /* Servidor */
    Serial.println("Iniciando o Servidor BLE");
    BLEDevice::init("BLE Bloco de notas");      
    pServer = BLEDevice::createServer();

    /* Criando o Serviço */
    pService = pServer->createService(SERVICE_UUID);
    
    /* Criando e Ajustando as caracteristicas */

    // CHAR_UUID_ADD_NOTA
    // CHAR_UUID_LISTAR_NOTAS
    // CHAR_UUID_NOTA_POR_INDICE    
    // CHAR_UUID_NOVA_NOTA_NOTIFY

    pCharac_add_nota = pService->createCharacteristic(
                          CHAR_UUID_ADD_NOTA,
                          BLECharacteristic::PROPERTY_WRITE 
                        //   | BLECharacteristic::PROPERTY_READ
                          );

    pCharac_listar_notas = pService->createCharacteristic(
                          CHAR_UUID_LISTAR_NOTAS,  
                          BLECharacteristic::PROPERTY_READ
                          );

    pCharac_nota_por_indice = pService->createCharacteristic(
                          CHAR_UUID_NOTA_POR_INDICE,  
                          BLECharacteristic::PROPERTY_READ  |
                          BLECharacteristic::PROPERTY_WRITE    | 
                          BLECharacteristic::PROPERTY_NOTIFY 
                          );

    pCharac_nova_nota_notify = pService->createCharacteristic(
                          CHAR_UUID_NOVA_NOTA_NOTIFY,  
                          BLECharacteristic::PROPERTY_NOTIFY 
                          );




    // Antigas:
    // pCharacteristic = pService->createCharacteristic(
    //                       CHARACTERISTIC_UUID,  
    //                       BLECharacteristic::PROPERTY_READ |
    //                       BLECharacteristic::PROPERTY_WRITE
    //                       );                  

    // /* Criando e Ajustando a caracteristica */
    // pCharac = pService->createCharacteristic(
    //                       CHARACTERISTIC_UUID2,
    //                       BLECharacteristic::PROPERTY_READ |
    //                       BLECharacteristic::PROPERTY_WRITE
    //                       );

    // /* No Setup */
    // notifyCharacteristic = pService->createCharacteristic(
    //                        CHARACTERISTIC_UUID3,
    //                        BLECharacteristic::PROPERTY_NOTIFY 
    //                        );

                            
    /* Descritores (para permitir notificação) */
    // O descritor 0x2902 é padrão para habilitar notificações
    pCharac_nova_nota_notify->addDescriptor(&notifyDescriptor);
    pCharac_nota_por_indice->addDescriptor(new BLEDescriptor(BLEUUID((uint16_t)0x2902))); // Pode reusar ou criar um novo

    /* Definido o valor inicial para a característica de "Listar" */
    pCharac_listar_notas->setValue("Nenhuma nota salva ainda");
    
    /* Definido o valor inicial para a característica de "Pegar nota pelo índice" */
    pCharac_nota_por_indice->setValue("Nenhum índice informado");
    

    /* Para registrar os Callbacks */
    // Registra o callback do servidor (para desconexão)
    pServer->setCallbacks(new MyServerCallbacks());
    // Registra o callback de escrita para as características que recebem dados
    pCharac_add_nota->setCallbacks(new MyCharacteristicCallbacks());
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
    



    // // Antigo:
    // pCharacteristic->setValue("Oi oi oi oi oi");
    // Serial.println("Caracteristica definida!");
    // notifyDescriptor.setValue("Valor incrementado");
    // notifyCharacteristic->addDescriptor(&notifyDescriptor);

    // /* Iniciando o Servico */
    // pService->start();

    // /* Criando e configurando o anuncio */
    // pAdvertising = BLEDevice::getAdvertising();
    // // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
    // pAdvertising->addServiceUUID(SERVICE_UUID);
    // pAdvertising->setScanResponse(true);
    // pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    // pAdvertising->setMinPreferred(0x12);
    // pCharacteristic->setCallbacks(new MyCharacteristicCallbacks());
    // pCharac->setCallbacks(new MyCharacteristicCallbacks());
    // pServer->setCallbacks(new MyServerCallbacks());                      
    // notifyDescriptor.setValue("Valor incrementado");
    // notifyCharacteristic->addDescriptor(&notifyDescriptor);

    // /* Inicia a anunciar */
    // BLEDevice::startAdvertising();
    // Serial.println("Anuncio configurado e iniciado. Pode-se conectar e ler do Cliente");
}

void loop() {
    delay(5000); // Delay pra abrir o monitor

}
