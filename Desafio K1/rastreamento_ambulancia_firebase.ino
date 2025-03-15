#include <WiFi.h>  // Biblioteca para conexão Wi-Fi
#include <HTTPClient.h>  // Biblioteca para fazer requisições HTTP
#include <TinyGPS++.h>  // Biblioteca para comunicação com o GPS
#include <FirebaseESP32.h>  // Biblioteca Firebase para ESP32

// Configuração do Wi-Fi
const char* ssid = "Starlink_CIT";  // Nome da rede Wi-Fi
const char* password = "Ufrr@2024Cit"; // Senha do Wi-Fi

// Configuração do Firebase
#define FIREBASE_HOST "https://rastreamento-de-ambulancia-default-rtdb.firebaseio.com"  // URL do Firebase
#define FIREBASE_AUTH "https://accounts.google.com/o/oauth2/auth"  // Token de autenticação do Firebase
// Configuração do GPS
#define RXD2 16  // Pino RX do ESP32 conectado ao TX do GPS
#define TXD2 17  // Pino TX do ESP32 conectado ao RX do GPS
#define GPS_BAUD 9600  // Taxa de comunicação do GPS
TinyGPSPlus gps;  // Instância do objeto GPS
HardwareSerial gpsSerial(2);  // Configuração da porta serial 2 para comunicação com o GPS

FirebaseData firebaseData;  // Instância do FirebaseData

void setup() {
    Serial.begin(115200);  // Inicia a comunicação serial para monitoramento
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);  // Inicia a comunicação com o módulo GPS
    Serial.println("Serial 2 do GPS iniciada");
    
    // Conecta-se ao Wi-Fi
    WiFi.begin(ssid, password);
    Serial.print("Conectando ao Wi-Fi");
    
    // Aguarda a conexão com o Wi-Fi
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi Conectado!");
    
    // Conecta-se ao Firebase
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
    Firebase.reconnectWiFi(true);  // Reconecta automaticamente ao Wi-Fi se desconectar
}

void loop() {
    unsigned long start = millis();  // Marca o tempo inicial
    bool newData = false;  // Variável para verificar se há novos dados do GPS
    
    // Lê os dados do GPS por 1 segundo
    while (millis() - start < 1000) {
        while (gpsSerial.available() > 0) {  // Se houver dados disponíveis na serial do GPS
            if (gps.encode(gpsSerial.read())) {  // Decodifica os dados recebidos
                newData = true;  // Marca que novos dados foram recebidos
            }
        }
    }
    
    // Se houver novos dados e a localização for válida
    if (newData && gps.location.isValid()) {
        float lat = gps.location.lat();  // Obtém a latitude
        float lon = gps.location.lng();  // Obtém a longitude
        
        // Exibe a localização no monitor serial
        Serial.print("Latitude: "); Serial.println(lat, 6);
        Serial.print("Longitude: "); Serial.println(lon, 6);

        // Verifica se o Wi-Fi está conectado antes de enviar os dados
        if (WiFi.status() == WL_CONNECTED) {
            // Cria o caminho do Firebase para salvar os dados de localização
            String path = "/ambulance_location/" + String(millis());
            
            // Preenche os dados no formato JSON
            String data = "{\"latitude\": " + String(lat, 6) + ", \"longitude\": " + String(lon, 6) + "}";

            // Envia os dados para o Firebase
            if (Firebase.setString(firebaseData, path, data)) {
                Serial.println("Localização enviada ao Firebase com sucesso!");
            } else {
                Serial.println("Falha ao enviar localização ao Firebase: " + firebaseData.errorReason());
            }
        } else {
            // Se o Wi-Fi estiver desconectado, tenta reconectar
            Serial.println("Wi-Fi desconectado, tentando reconectar...");
            WiFi.begin(ssid, password);
        }
    } else {
        Serial.println("Aguardando coordenadas válidas do GPS...");
    }
    
    delay(5000); // Aguarda 5 segundos antes de enviar novamente
}
