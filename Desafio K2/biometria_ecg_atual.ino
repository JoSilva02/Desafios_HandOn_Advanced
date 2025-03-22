#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Starlink_CIT";  // Substitua pelo seu SSID
const char* password = "Ufrr@2024Cit";  // Substitua pela sua senha Wi-Fi

HardwareSerial mySerial(1); // Usando UART1 do ESP32 (GPIO16, GPIO17)
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

int ecgPin = 15;
bool autenticado = false;
int valorECG = 0;

String servidorURL = "http://192.168.1.172:5000/dados";  // URL do servidor

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    mySerial.begin(57600, SERIAL_8N1, 16, 17); // Configuração UART para biometria
    pinMode(ecgPin, INPUT);  // Pino do ECG configurado como entrada

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi!");
  Serial.print("IP_Address: ");
  Serial.println(WiFi.localIP());
  
  finger.begin(57600);
  if (finger.verifyPassword()) {
      Serial.println("Sensor de biometria detectado!");
  } else {
      Serial.println("Falha ao encontrar sensor de biometria.");
      while (1);
  }
}

void loop() {

  String jsonPayload = "{\"ecg\": " + String(valorECG) + "}";
  Serial.println("Payload JSON enviado: " + jsonPayload);  // Verifique no Serial Monitor se está correto

    if (!autenticado) {
        Serial.println("Aguardando autenticação...");
        autenticado = verificarDigital();
    }

    if (autenticado) {
        valorECG = analogRead(ecgPin);
        Serial.println("Iniciando leitura do batimento cardíaco...");
        Serial.println("ECG: " + String(valorECG));
        delay(100);
    }

  // Enviar dados para o servidor
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(servidorURL);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(5000);  // 5000 ms = 5 segundos
    int httpResponseCode = http.POST(jsonPayload);


    if (httpResponseCode > 0) {
      Serial.println("Dados enviados com sucesso!");
      String response = http.getString();  // Lê a resposta do servidor
      Serial.println("Resposta do servidor: " + response);
    } else {
      Serial.println("Erro ao enviar dados: " + String(httpResponseCode));
    }
    http.end();
  } else {
    Serial.println("WiFi desconectado!");
  }
}

// Função para validar a digital
bool verificarDigital() {
    Serial.println("Posicione o dedo...");
    int id = finger.getImage();

    if (id != FINGERPRINT_OK) return false;

    id = finger.image2Tz();
    if (id != FINGERPRINT_OK) return false;

    id = finger.fingerFastSearch();
    if (id == FINGERPRINT_OK) {
        Serial.println("Usuário autenticado!");
        return true;
    } else {
        Serial.println("Autenticação falhou!");
        return false;
    }
}

/*void enviarDadosServidor(String jsonPayload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(servidorURL);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.println("Dados enviados com sucesso!");
    } else {
      Serial.println("Erro ao enviar dados: " + String(httpResponseCode));
    }
    http.end();
  }
}*/