#include <WiFi.h>
#include <DHT.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>

#define DHT_PIN 14

const char* ssid = "Benjamin Guntoro";
const char* password = "20052006";

float suhu = 0.0;
float kelembapanTanah = 0.0;

DHT dht(DHT_PIN, DHT11);
AsyncWebServer server(80);

void setup(){
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println(WiFi.localIP());

    const char home_page[] = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>SmartHome ESP32</title>

  <!-- Tailwind CSS -->
  <script src="https://cdn.tailwindcss.com"></script>

  <!-- Font Awesome -->
  <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css" />

  <!-- Google Fonts: Open Sans, Roboto, dan Press Start 2P (Matrix) -->
  <link href="https://fonts.googleapis.com/css2?family=Open+Sans:wght@300;400;600&family=Roboto:wght@300;400;500&family=Press+Start+2P&display=swap" rel="stylesheet">

  <style>
    body {
      font-family: 'Open Sans', sans-serif;
    }

    .matrix-font {
      font-family: 'Press Start 2P', cursive;
      letter-spacing: 2px;
    }
  </style>
</head>
<body class="bg-gradient-to-br from-slate-100 to-slate-200 min-h-screen text-gray-800">

  <div class="max-w-md mx-auto p-4 pb-24">
    <!-- Header -->
    <header class="text-center mb-6">
      <h1 class="text-3xl font-semibold tracking-wide">SmartHome 1.0</h1>
      <p class="text-sm text-gray-500 mt-1">Made by 0xbdg</p>
    </header>

    <!-- Card: Lampu Toggle -->
    <div class="bg-white rounded-3xl shadow-md p-5 mb-5 flex items-center justify-between">
      <div>
        <h2 class="text-lg font-medium text-gray-700 mb-1">Lampu</h2>
        <div class="flex items-center gap-2">
          <i class="fas fa-lightbulb text-yellow-400 text-lg"></i>
          <span id="lamp-status" class="text-sm font-semibold text-green-500">Nyala</span>
        </div>
      </div>
      <label class="relative inline-flex items-center cursor-pointer scale-110">
        <input type="checkbox" value="" id="lamp-toggle" class="sr-only peer" checked>
        <div class="w-12 h-7 bg-gray-300 rounded-full peer peer-checked:bg-blue-600 transition-colors duration-300"></div>
        <div class="absolute left-1 top-1 w-5 h-5 bg-white rounded-full shadow-md transition-transform duration-300 peer-checked:translate-x-5"></div>
      </label>
    </div>

    <!-- Card: Suhu -->
    <div class="bg-white rounded-3xl shadow-md p-5 mb-5 flex items-center justify-between">
      <div class="flex items-center gap-4">
        <i class="fas fa-temperature-high text-rose-500 text-2xl"></i>
        <div class="flex flex-col">
          <span class="text-sm text-gray-500">Suhu</span>
          <span id="suhu" class="text-2xl text-gray-800 matrix-font">>--°C</span>
        </div>
      </div>
    </div>

    <!-- Card: Kelembapan Tanah -->
    <div class="bg-white rounded-3xl shadow-md p-5 flex items-center justify-between">
      <div class="flex items-center gap-4">
        <i class="fas fa-tint text-sky-500 text-2xl"></i>
        <div class="flex flex-col">
          <span class="text-sm text-gray-500">Kelembapan Tanah</span>
          <span id="kelembapan" class="text-2xl text-gray-800 matrix-font">>--%</span>
        </div>
      </div>
    </div>
  </div>

  <!-- Bottom Navigation Bar -->
  <nav class="fixed bottom-0 left-0 right-0 bg-white border-t shadow-lg z-10">
    <div class="max-w-md mx-auto flex justify-around py-3 text-gray-600">
      <button class="flex flex-col items-center text-blue-600">
        <i class="fas fa-home text-xl"></i>
        <span class="text-xs mt-1">Beranda</span>
      </button>
      <button class="flex flex-col items-center">
        <i class="fas fa-list text-xl"></i>
        <span class="text-xs mt-1">Log</span>
      </button>
      <button class="flex flex-col items-center">
        <i class="fas fa-cog text-xl"></i>
        <span class="text-xs mt-1">Pengaturan</span>
      </button>
    </div>
  </nav>

  <!-- Toggle Lampu Script -->
  <script>
    const lampToggle = document.getElementById("lamp-toggle");
    const lampStatus = document.getElementById("lamp-status");

    lampToggle.addEventListener("change", () => {
      const isOn = lampToggle.checked;
      lampStatus.textContent = isOn ? "Nyala" : "Mati";
      lampStatus.classList.toggle("text-green-500", isOn);
      lampStatus.classList.toggle("text-red-500", !isOn);

      // Simulasi kirim ke ESP32
      console.log("Lampu: ", isOn ? "ON" : "OFF");
      fetch('/toggle', {
          method: 'POST',
          headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
          body: 'state=' + (isOn ? 'ON' : 'OFF')
       });

    });
    const suhuElem = document.getElementById("suhu");
    const kelembapanElem = document.getElementById("kelembapan");

    function updateSensorData() {
      fetch("/data")
        .then(res => res.json())
        .then(data => {
          suhuElem.textContent = `${data.suhu}°C`;
          kelembapanElem.textContent = `${data.kelembapan_tanah}%`;
        });
    }

    setInterval(updateSensorData, 1000); // update tiap 5 detik
    updateSensorData(); // panggil pertama kali
  </script>

</body>
</html>

    )rawliteral";

    server.on("/", HTTP_GET, [home_page](AsyncWebServerRequest *request){
      request->send(200, "text/html", home_page);
    });

    server.on("/toggle", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("state", true)) {
      String state = request->getParam("state", true)->value();
      Serial.println("Lampu: " + state);
      if (state == "ON"){
        digitalWrite(LED_BUILTIN, HIGH);
      }
      else if (state == "OFF"){
        digitalWrite(LED_BUILTIN, LOW);
      }
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Parameter tidak ditemukan");
    }
    });
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request){
    String json = "{\"suhu\":" + String(suhu, 1) + 
                  ",\"kelembapan_tanah\":" + String(kelembapanTanah) + "}";
    request->send(200, "application/json", json);
    });

    server.begin();
}

void loop(){
  
}
