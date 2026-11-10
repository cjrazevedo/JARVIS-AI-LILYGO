#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <TFT_eSPI.h>

static const char* AP_NAME = "JARVIS-LILYGO";
static const char* AP_PASS = "jarvis123";
static const char* AI_URL = "https://api.openai.com/v1/chat/completions";

TFT_eSPI tft;
WebServer server(80);
Preferences prefs;
String apiKey;
String lastAnswer = "Aguardando comando...";
String statusText = "Iniciando";

const char PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="pt-BR"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>JARVIS AI</title><style>
body{font-family:Arial;background:#07111f;color:#e8f5ff;margin:0;padding:18px}
main{max-width:650px;margin:auto}h1{color:#27d9ff}textarea,input{box-sizing:border-box;width:100%;padding:13px;margin:8px 0;border-radius:9px;border:1px solid #28506b;background:#102238;color:white}
button{padding:13px 16px;border:0;border-radius:9px;background:#00a9d6;color:#00121b;font-weight:bold;margin:5px 4px 5px 0}
#answer{white-space:pre-wrap;background:#102238;border-radius:10px;padding:14px;min-height:100px}
small{color:#9db6c9}
</style><main><h1>◉ JARVIS AI</h1><small>Assistente conectado ao LILYGO T-Embed</small>
<input id="key" type="password" placeholder="Cole sua chave da API OpenAI (salva somente no ESP32)">
<button onclick="saveKey()">Salvar chave</button>
<textarea id="q" rows="3" placeholder="Digite ou use o microfone..."></textarea>
<button onclick="send()">Enviar</button><button onclick="listen()">🎙 Falar</button><button onclick="speak()">🔊 Repetir resposta</button>
<h3>Resposta</h3><div id="answer">Pronto para conversar.</div>
<p><small>O reconhecimento e a fala usam o navegador do Android. A IA responde pela internet.</small></p>
<script>
let last='';
async function saveKey(){let k=document.getElementById('key').value.trim();if(!k){alert('Cole a chave primeiro.');return;}
let r=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({api_key:k})});
let j=await r.json();alert(j.message||j.error||'');document.getElementById('key').value='';}
async function send(){let q=document.getElementById('q').value.trim();if(!q)return;
document.getElementById('answer').textContent='JARVIS está pensando...';
try{let r=await fetch('/ask',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({question:q})});
let j=await r.json();last=j.answer||j.error||'Sem resposta';document.getElementById('answer').textContent=last;speak();}
catch(e){document.getElementById('answer').textContent='Erro de conexão: '+e;}}
function listen(){let R=window.SpeechRecognition||window.webkitSpeechRecognition;if(!R){alert('Use Google Chrome atualizado no Android.');return;}
let rec=new R();rec.lang='pt-BR';rec.interimResults=false;rec.onresult=e=>{document.getElementById('q').value=e.results[0][0].transcript;send();};rec.onerror=e=>alert('Microfone: '+e.error);rec.start();}
function speak(){if(last) speechSynthesis.speak(new SpeechSynthesisUtterance(last));}
</script></main></html>
)HTML";

void drawScreen(const String& line1, const String& line2 = "") {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(8, 8); tft.println("JARVIS AI");
  tft.drawFastHLine(8, 34, 154, TFT_DARKCYAN);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(8, 48); tft.println(line1);
  if (line2.length()) { tft.setCursor(8, 68); tft.println(line2); }
}

void handleRoot() {
  server.send_P(200, "text/html; charset=utf-8", PAGE);
}

void handleConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Dados ausentes\"}");
    return;
  }
  JsonDocument cfg;
  if (deserializeJson(cfg, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"JSON invalido\"}");
    return;
  }
  String key = cfg["api_key"] | "";
  key.trim();
  if (!key.startsWith("sk-") || key.length() < 20) {
    server.send(400, "application/json", "{\"error\":\"Chave parece invalida\"}");
    return;
  }
  prefs.putString("api_key", key);
  apiKey = key;
  server.send(200, "application/json", "{\"message\":\"Chave salva no ESP32\"}");
}

void handleAsk() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Corpo vazio\"}");
    return;
  }
  JsonDocument incoming;
  if (deserializeJson(incoming, server.arg("plain"))) {
    server.send(400, "application/json", "{\"error\":\"JSON invalido\"}");
    return;
  }
  String question = incoming["question"] | "";
  question.trim();
  if (!question.length()) {
    server.send(400, "application/json", "{\"error\":\"Digite uma pergunta\"}");
    return;
  }
  if (!apiKey.length()) {
    server.send(503, "application/json", "{\"error\":\"Configure a chave de API no portal JARVIS-SETUP\"}");
    return;
  }

  drawScreen("Ouvindo pergunta:", question.substring(0, 26));
  WiFiClientSecure client;
  client.setInsecure(); // Protótipo: validação TLS será endurecida na versão de produção.
  HTTPClient https;
  if (!https.begin(client, AI_URL)) {
    server.send(500, "application/json", "{\"error\":\"Falha ao iniciar HTTPS\"}");
    return;
  }
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Authorization", "Bearer " + apiKey);

  JsonDocument req;
  req["model"] = "gpt-4o-mini";
  req["temperature"] = 0.6;
  JsonArray messages = req["messages"].to<JsonArray>();
  JsonObject sys = messages.add<JsonObject>();
  sys["role"] = "system";
  sys["content"] = "Seu nome e JARVIS. Responda sempre em portugues brasileiro, de forma clara, util e objetiva. Nao afirme que executou acoes que nao executou.";
  JsonObject usr = messages.add<JsonObject>();
  usr["role"] = "user";
  usr["content"] = question;
  String payload;
  serializeJson(req, payload);

  int code = https.POST(payload);
  String body = https.getString();
  https.end();

  if (code < 200 || code >= 300) {
    String msg = "Erro da API (" + String(code) + "). Verifique a chave, saldo e internet.";
    drawScreen("Erro na API", String(code));
    JsonDocument out; out["error"] = msg; String response; serializeJson(out, response);
    server.send(502, "application/json", response);
    return;
  }

  JsonDocument result;
  if (deserializeJson(result, body)) {
    server.send(502, "application/json", "{\"error\":\"Resposta da IA invalida\"}");
    return;
  }
  lastAnswer = result["choices"][0]["message"]["content"] | "Nao consegui obter resposta.";
  drawScreen("JARVIS respondeu:", lastAnswer.substring(0, 26));
  JsonDocument out; out["answer"] = lastAnswer;
  String response; serializeJson(out, response);
  server.send(200, "application/json; charset=utf-8", response);
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  drawScreen("Inicializando Wi-Fi...");
  prefs.begin("jarvis", false);
  apiKey = prefs.getString("api_key", "");

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  bool connected = wm.autoConnect("JARVIS-SETUP", "jarvis123");
  if (!connected) {
    drawScreen("Wi-Fi nao configurado", "Reinicie para tentar");
    delay(1500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    drawScreen("Wi-Fi conectado", WiFi.localIP().toString());
  } else {
    drawScreen("Sem Wi-Fi", "Portal JARVIS-SETUP");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/ask", HTTP_POST, handleAsk);
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/status", HTTP_GET, []() {
    JsonDocument d; d["wifi"] = WiFi.status() == WL_CONNECTED;
    d["ip"] = WiFi.localIP().toString(); d["api_key_configured"] = apiKey.length() > 0;
    String s; serializeJson(d, s); server.send(200, "application/json", s);
  });
  server.begin();
}

void loop() {
  server.handleClient();
  delay(2);
}
