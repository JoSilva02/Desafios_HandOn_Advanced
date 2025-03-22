#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <HeartRateSensor.h> // Substitua com a biblioteca correta
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Starlink_CIT";  // Substitua pelo seu SSID
const char* password = "Ufrr@2024Cit";  // Substitua pela sua senha Wi-Fi

HeartRateSensor particleSensor;
int ecgPin = 34;
int valorECG = 0;

HardwareSerial mySerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

String servidorURL = "http://192.168.1.147:5000/dados";  // URL do servidor

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Espera até conectar ao Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFi...");
  }
  Serial.println("Conectado ao WiFi!");
  Serial.print("IP_Address: ");
  Serial.println(WiFi.localIP());
  
  mySerial.begin(57600, SERIAL_8N1, 16, 17);

  finger.begin(57600);
  if (!finger.verifyPassword()) {
    Serial.println("Erro: Sensor não encontrado!");
    while (1);
  }

  // Inicializando o sensor de batimento cardíaco
  if (!particleSensor.begin()) {
    Serial.println("Erro ao iniciar o sensor de batimentos cardíacos.");
    while (1);
  }
}

void loop() {
  valorECG = analogRead(ecgPin);
  Serial.println(valorECG);

  // Criar JSON com os dados do ECG
  String jsonPayload = "{\"ecg\": " + String(valorECG) + "}";

  // Enviar dados para o servidor
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

  delay(100);  // Delay para a próxima leitura

  Serial.println("Posicione o dedo para autenticação...");
  int id = verificarDigital();

  if (id > 0) {
    Serial.print("Usuário autenticado! ID: ");
    Serial.println(id);

    // Leitura dos batimentos cardíacos
    Serial.println("Iniciando leitura do batimento cardíaco...");
    int batimentoCardiaco = lerBatimentoCardiaco();
    
    if (batimentoCardiaco >= 0) {
      Serial.print("Batimentos cardíacos: ");
      Serial.println(batimentoCardiaco);
    } else {
      Serial.println("Falha ao ler batimentos cardíacos.");
    }
    
    // Aqui você pode acionar outras funções, como abrir portas, etc.
  } else {
    Serial.println("Falha na autenticação!");
  }

  delay(2000);
}

int verificarDigital() {
  int resultado = finger.getImage();
  if (resultado != FINGERPRINT_OK) return -1;

  resultado = finger.image2Tz();
  if (resultado != FINGERPRINT_OK) return -1;

  resultado = finger.fingerFastSearch();
  if (resultado == FINGERPRINT_OK) {
    return finger.fingerID;  // Retorna o ID do usuário autenticado
  }

  return -1;  // Falha na autenticação
}

int lerBatimentoCardiaco() {
  long startMillis = millis();

  // Loop para esperar por um valor válido de batimento cardíaco
  while (millis() - startMillis < 10000) {  // Aguarda 10 segundos para pegar um valor
    if (particleSensor.available()) {
      long irValue = particleSensor.getIR();  // Lê o valor do sensor IR

      if (irValue > 50000) {  // Um valor de IR maior sugere um dedo detectado no sensor
        int bpm = particleSensor.getHeartRate();  // Calcula os batimentos por minuto
        return bpm;
      }
    }
  }

  return -1;  // Falha na leitura do batimento cardíaco
}
