 #include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <Arduino.h>
#include <TimeLib.h>

// Definisikan pin
#define ZMPT_PIN 34     
#define VOLTAGE_PIN 35  
#define CURRENT_PLN_PIN 32 
#define CURRENT_ACCU_PIN 33 
#define DHT_PIN 4      

#define DHT_TYPE DHT22 

// WiFi settings
const char *ssid = "SSID";       
const char *password = "password"; 

DHT dht(DHT_PIN, DHT_TYPE);

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 7 * 3600, 60000); // Set timezone to UTC+7

float voltagePLN = 0.0;
float voltageACCU = 0.0;
float currentPLN = 0.0;
float currentACCU = 0.0;
float temperature = 0.0;
int batteryPercentage = 100; 
float energyConsumed = 0.0;  
String statusPLN = "Normal";
String statusACCU = "Normal";
String message = "";
String batteryStatus = "";  // Status baterai ACCU

String lokasiID = "1";          
String sumberDayaPLN_ID = "1";  
String sumberDayaACCU_ID = "2"; 

unsigned long monitoringID = 1;    
unsigned long logID = 1;        
unsigned long konsumsidayaID = 1; 
unsigned long notificationID = 1; 

// Kapasitas baterai ACCU dalam Ah
const float batteryCapacity = 36.0; // Kapasitas 36 Ah

// ID unik
unsigned long dataID = 1;  // ID unik mulai dari 1

float readVoltage(int pin) {
    int rawValue = analogRead(pin);
    float voltage = (rawValue / 4095.0) * 3.3; 
    return voltage;
}

float readCurrent(int pin) {
    int rawValue = analogRead(pin);
    float voltage = (rawValue / 4095.0) * 3.3; 
    float current = (voltage - 2.5) / 0.185;   
    return current;
}

void sendDataToServer() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        // Kirim data ke server pertama
        http.begin("http:// 192.168.1.17/Webdashboard/api/monitoring.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData = "monitoring_id=" + String(monitoringID) +
                          "&log_id=" + String(logID) +
                          "&sumberdaya_listrik_id="+ String(sumberDayaPLN_ID) + 
                          "&sumberdaya_accu_id="+ String(sumberDayaACCU_ID) + 
                          "&lokasi_id=" + lokasiID +
                          "&konsumsidaya_id=" + String(konsumsidayaID) +
                          "&tegangan_listrik=" + String(voltagePLN) +
                          "&arus_listrik=" + String(currentPLN) +
                          "&tegangan_accu=" + String(voltageACCU) +
                          "&arus_accu=" + String(currentACCU) +
                          "&suhu=" + String(temperature) +
                          "&SoC=" + String(batteryPercentage) +
                          "&SoE=" + String(energyConsumed);

        int httpResponseCode = http.POST(postData);

        Serial.println("Mengirim data ke server...");
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.print("Response: ");
            Serial.println(response);
        } else {
            Serial.print("Error mengirim data: ");
            Serial.println(httpResponseCode);
        }

        http.end();
        
        // Kirim data ke server kedua (MySQL)
        http.begin("http://192.168.1.17/Webdashboard/api/realtimedata.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postDataMySQL = "data_id=" + String(dataID) +
                               "&log_id=" + String(logID) +
                               "&monitoring_id=" + String(monitoringID) +
                               "&konsumsidaya_id=" + String(konsumsidayaID);

        int httpResponseCodeMySQL = http.POST(postDataMySQL);

        Serial.println("Mengirim data ke database MySQL...");
        if (httpResponseCodeMySQL > 0) {
            String response = http.getString();
            Serial.print("Response: ");
            Serial.println(response);
        } else {
            Serial.print("Error mengirim data ke MySQL: ");
            Serial.println(httpResponseCodeMySQL);
        }

        http.end();
    } else {
        Serial.println("WiFi tidak terhubung.");
    }
}

void sendConsumptionData() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        http.begin("http://192.168.1.17/Webdashboard/api/konsumsidaya.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData = "konsumsidaya_id=" + String(konsumsidayaID) +
                          "&log_id=" + String(logID) +
                          "&monitoring_id=" + String(monitoringID) +
                          "&estimasi=" + String(estimateBatteryLife(currentACCU)) +
                          "&status=" + batteryStatus;

        int httpResponseCode = http.POST(postData);

        Serial.println("Mengirim data konsumsi daya...");
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.print("Response: ");
            Serial.println(response);
        } else {
            Serial.print("Error mengirim data konsumsi daya: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi tidak terhubung.");
    }
}

void sendActivityLog() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        http.begin("http://192.168.1.17/Webdashboard/api/logaktivitas.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData = "log_id=" + String(logID) +
                          "&monitoring_id=" + String(monitoringID) +
                          "&lokasi_id=" + lokasiID +
                          "&sumberdaya_listrik_id="+ String(sumberDayaPLN_ID) + 
                          "&sumberdaya_accu_id="+ String(sumberDayaACCU_ID) + 
                          "&konsumsidaya_id=" + String(konsumsidayaID) +
                          "&waktu=" + String(timeClient.getFormattedTime()) +
                          "&status_listrik=" + statusPLN + 
                          "&status_listrik=" + statusACCU;

        int httpResponseCode = http.POST(postData);

        Serial.println("Mengirim log aktivitas...");
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.print("Response: ");
            Serial.println(response);
        } else {
            Serial.print("Error mengirim log aktivitas: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi tidak terhubung.");
    }
}

void sendNotification() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        http.begin("http://192.168.1.17/Webdashboard/api/notifikasi.php");
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String postData = "notifikasi_id=" + String(notificationID) +
                          "&log_id=" + String(logID) +
                          "&lokasi_id=" + lokasiID +
                          "&konsumsidaya_id=" + String(konsumsidayaID) +
                          "&monitoring_id=" + String(monitoringID) +
                          "&pesan=" + message;

        int httpResponseCode = http.POST(postData);

        Serial.println("Mengirim notifikasi...");
        if (httpResponseCode > 0) {
            String response = http.getString();
            Serial.print("Response: ");
            Serial.println(response);
        } else {
            Serial.print("Error mengirim notifikasi: ");
            Serial.println(httpResponseCode);
        }

        http.end();
    } else {
        Serial.println("WiFi tidak terhubung.");
    }
}

float estimateBatteryLife(float current) {
    // Menghitung estimasi waktu bertahan baterai dalam jam
    if (current > 0) {
        return batteryCapacity / current; 
    } else {
        return 0; // Jika arus 0, waktu bertahan tidak bisa dihitung
    }
}

void setup() {
    Serial.begin(115200);

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    dht.begin();

    pinMode(ZMPT_PIN, INPUT);
    pinMode(VOLTAGE_PIN, INPUT);
    pinMode(CURRENT_PLN_PIN, INPUT);
    pinMode(CURRENT_ACCU_PIN, INPUT);

    timeClient.begin();
}

void loop() {
    timeClient.update();

    voltagePLN = readVoltage(ZMPT_PIN);
    voltageACCU = readVoltage(VOLTAGE_PIN);
    currentPLN = readCurrent(CURRENT_PLN_PIN);
    currentACCU = readCurrent(CURRENT_ACCU_PIN);
    temperature = dht.readTemperature();

    if (voltagePLN < 190) {
        statusPLN = "Turun";
    } else {
        statusPLN = "Normal";
    }

    if (voltageACCU < 11) {
        statusACCU = "Turun";
    } else {
        statusACCU = "Normal";
    }

    if (voltageACCU <= 11) {
        batteryPercentage = map(voltageACCU, 11, 12, 0, 100);
    } else {
        batteryPercentage = 100;
    }

    // Menentukan status baterai ACCU
    if (batteryPercentage <= 20) {
        batteryStatus = "Hampir Habis";
    } else if (batteryPercentage >= 50 && batteryPercentage <= 70) {
        batteryStatus = "Cukup";
    } else if (batteryPercentage > 70) {
        batteryStatus = "Penuh";
    } else {
        batteryStatus = "Normal";  // Status normal jika berada di luar rentang tersebut
    }

    energyConsumed = (voltageACCU * currentACCU) / 1000.0; 

    if (batteryPercentage <= 20) {
        message = "Daya ACCU habis!";
    } else {
        message = "Normal";
    }
    
    float batteryLife = estimateBatteryLife(currentACCU);

    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime((time_t *)&epochTime);

    String currentDateTime = String(ptm->tm_mday) + "/" + String(ptm->tm_mon + 1) + "/" +
                             String(ptm->tm_year + 1900) + " " +
                             String(ptm->tm_hour) + ":" + String(ptm->tm_min) + ":" + String(ptm->tm_sec);

    Serial.println("===================================");
    Serial.print("Monitoring ID: ");
    Serial.println(monitoringID); 
    Serial.print("Log ID: ");
    Serial.println(logID);
    Serial.print("Konsumsi Daya ID: ");
    Serial.println(konsumsidayaID);
    Serial.print("Lokasi ID: ");
    Serial.println(lokasiID);
    Serial.println("Route: Route utama");
    Serial.println();
    Serial.print("Sumber Daya PLN ID: ");
    Serial.println(sumberDayaPLN_ID);
    Serial.print("Sumber Daya ACCU ID: ");
    Serial.println(sumberDayaACCU_ID);
    Serial.println("===================================");

    Serial.print("Tanggal dan Waktu: ");
    Serial.println(currentDateTime);
    Serial.print("Tegangan PLN: ");
    Serial.print(voltagePLN);
    Serial.print("V - Status PLN: ");
    Serial.println(statusPLN);
    Serial.print("Tegangan ACCU: ");
    Serial.print(voltageACCU);
    Serial.print("V - Status ACCU: ");
    Serial.println(statusACCU);
    Serial.print("Arus PLN: ");
    Serial.print(currentPLN);
    Serial.println("A");
    Serial.print("Arus ACCU: ");
    Serial.print(currentACCU);
    Serial.println("A");
    Serial.print("Suhu: ");
    Serial.print(temperature);
    Serial.println("°C");
    Serial.print("Status Baterai: ");
    Serial.println(batteryStatus);
    Serial.print("Persentase Baterai ACCU: ");
    Serial.print(batteryPercentage);
    Serial.println("%");
    Serial.print("Energi yang digunakan (kWh): ");
    Serial.println(energyConsumed);
    Serial.print("Pesan: ");
    Serial.println(message);
    Serial.print("Estimasi Waktu Bertahan Baterai (jam): ");
    Serial.println(batteryLife);
    Serial.print("Notifikasi ID: ");
    Serial.println(notificationID);
    Serial.print("Data_id: ");
    Serial.println(dataID);
    Serial.println("===================================");

    // Kirim data ke server
    sendDataToServer();
    // Kirim data konsumsi daya
    sendConsumptionData();

    // Kirim log aktivitas
    sendActivityLog();

    // Kirim notifikasi
    sendNotification();

    monitoringID++;
    logID++;
    konsumsidayaID++;
    notificationID++;  // Menambahkan notifikasi_id unik
    dataID++;  // Menambahkan dataID unik setiap loop

    delay(2000);
}
