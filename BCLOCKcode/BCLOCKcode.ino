//https://github.com/Jendys777/BCLOCK
//Created by Bojler777
//This is my desing of code for the LED BCLOCK. Be free to tinker!
//if you have questions feel free to mail me on: info@bojler.org

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>

#define PIN        4 
#define NUMPIXELS  30  
#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"

// --- WIFI SETTINGS ---
const char* wifiname = "Your_WiFi_Name";     // Enter your SSID here
const char* wifipassword = "Your_Password";  // Enter your WiFi password here
// ---------------------

struct Config {
  byte r = 255, g = 0, b = 0;
  int nightStart = 23, nightEnd = 6, brightDay = 50, brightNight = 5;
  byte visualMode = 0, appMode = 0, speed = 50;
  int timerVal = 10;
  bool blanking = false; 
} conf;

unsigned long swStart = 0, swElapsed = 0;
bool swRunning = false;
unsigned long timerStart = 0, timerRemaining = 0;
bool timerRunning = false, timerFinished = false;
int snakePos = 0;
bool stroboState = false;

Adafruit_NeoPixel strip(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(80);
const byte digitMap[] = { 0x77, 0x44, 0x3E, 0x6E, 0x4D, 0x6B, 0x7B, 0x46, 0x7F, 0x6F };
unsigned long lastUpdate = 0;
uint32_t effectHue = 0;

String getHexColor() {
  char hex[8]; sprintf(hex, "#%02X%02X%02X", conf.r, conf.g, conf.b);
  return String(hex);
}

String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no'>";
  html += "<link rel='apple-touch-icon' href='https://img.icons8.com/fluency/180/clock.png'><meta name='apple-mobile-web-app-capable' content='yes'><title>B-CLOCK v3.0</title>";
  html += "<style>body{font-family:-apple-system,sans-serif; background:#000; color:#eee; margin:0; display:flex; flex-direction:column; align-items:center; padding:15px;} ";
  html += ".card{background:#1c1c1e; padding:20px; border-radius:25px; width:100%; max-width:360px; box-sizing:border-box; margin-bottom:12px; text-align:center;} ";
  html += "h2{margin:5px 0; font-size:20px; letter-spacing:2px;} h3{font-size:12px; color:#ff9500; margin:0 0 10px 0; text-transform:uppercase;} ";
  html += "input, select{width:100%; padding:14px; border-radius:12px; border:none; background:#2c2c2e; color:white; font-size:16px; margin:5px 0; -webkit-appearance:none; box-sizing:border-box; text-align:center;} ";
  html += "input[type='range']{-webkit-appearance:none; background:transparent; margin:10px 0; height:30px;} ";
  html += "input[type='range']::-webkit-slider-runnable-track{height:8px; background:#3a3a3c; border-radius:4px;} ";
  html += "input[type='range']::-webkit-slider-thumb{-webkit-appearance:none; height:24px; width:24px; background:#fff; border:2px solid #ff9500; border-radius:50%; margin-top:-8px;} ";
  html += ".grid{display:grid; grid-template-columns: 1fr 1fr; gap:8px;} ";
  html += ".btn-ctrl{background:#ff9500; color:black; font-weight:bold; border-radius:12px; padding:12px; text-decoration:none; font-size:22px; display:block;} ";
  html += ".chk-row{display:flex; align-items:center; justify-content:space-between; background:#2c2c2e; padding:12px 15px; border-radius:12px; margin:8px 0;} ";
  html += "input[type='checkbox']{width:26px; height:26px; margin:0; background:#444; border-radius:6px; -webkit-appearance:checkbox;} ";
  html += "label{font-size:10px; color:#8e8e93; text-transform:uppercase; font-weight:700; display:block; margin-top:15px; text-align:left;} ";
  html += "span.val{float:right; color:#ff9500;}</style>";
  
  html += "<script>function update(el){ ";
  html += "let val = el.type=='checkbox' ? (el.checked?1:0) : el.value; ";
  html += "if(document.getElementById(el.name+'_v')) document.getElementById(el.name+'_v').innerText = val+'%'; ";
  html += "if(el.type=='color') val = val.replace('#','%23'); ";
  html += "fetch('/save?'+el.name+'='+val); }</script></head><body>";
  
  html += "<h2>📟 B-CLOCK 📟</h2>";

  // Control Buttons
  if(conf.appMode == 1 || conf.appMode == 2 || conf.appMode >= 5) {
    html += "<div class='card'><h3>Control Panel</h3><div style='display:flex; justify-content:space-around;'>";
    html += "<a href='/ctrl?a=start' class='btn-ctrl' style='width:30%'>&#9654;</a>";
    html += "<a href='/ctrl?a=stop' class='btn-ctrl' style='width:30%; background:#ff3b30; color:white;'>&#9632;</a>";
    html += "<a href='/ctrl?a=reset' class='btn-ctrl' style='width:30%; background:#444; color:white;'>&#8634;</a>";
    html += "</div></div>";
  }

  html += "<div class='card'><h3>Settings</h3>";

  // Timer values nahoře
  if(conf.appMode >= 5) { 
    html += "<label>Timer Set Value ("+String(conf.appMode==5?"Sec":"Min")+")</label><input type='number' name='tv' value='"+String(conf.timerVal)+"' onchange='update(this)'>"; 
  }

  html += "<label>Mode</label><select name='am' onchange='update(this); setTimeout(()=>location.reload(), 150)'>";
  const char* apps[] = {"Clock", "Stopwatch (SS:CC)", "Stopwatch (MM:SS)", "Snake", "Stroboscope", "Timer (Sec)", "Timer (Min)"};
  for(int i=0; i<7; i++) html += "<option value='"+String(i)+"' "+(conf.appMode==i?"selected":"")+">"+apps[i]+"</option>";
  html += "</select>";

  html += "<div class='chk-row'><b>Zero Blanking</b><input type='checkbox' name='bl' "+String(conf.blanking?"checked":"")+" onchange='update(this)'></div>";

  html += "<label>Effect</label><select name='m' onchange='update(this)'>";
  const char* vis[] = {"Static", "Rainbow Cycle", "Rainbow Wave", "Fireplace", "Deep Purple", "Breathing"};
  for(int i=0; i<6; i++) html += "<option value='"+String(i)+"' "+(conf.visualMode==i?"selected":"")+">"+vis[i]+"</option>";
  html += "</select>";

  html += "<label>Speed <span class='val' id='sp_v'>"+String(conf.speed)+"%</span></label><input type='range' name='sp' min='1' max='100' value='"+String(conf.speed)+"' oninput='update(this)'>";
  html += "<label>Color</label><input type='color' name='c' value='"+getHexColor()+"' onchange='update(this)' style='height:50px; padding:5px;'>";
  html += "<label>Day Brightness <span class='val' id='bd_v'>"+String(conf.brightDay)+"%</span></label><input type='range' name='bd' min='1' max='100' value='"+String(conf.brightDay)+"' oninput='update(this)'>";
  html += "<label>Night Brightness <span class='val' id='bn_v'>"+String(conf.brightNight)+"%</span></label><input type='range' name='bn' min='1' max='100' value='"+String(conf.brightNight)+"' oninput='update(this)'>";
  
  html += "<label>Night Schedule</label><div class='grid'><input type='number' name='ns' value='"+String(conf.nightStart)+"' onchange='update(this)'><input type='number' name='ne' value='"+String(conf.nightEnd)+"' onchange='update(this)'></div>";

  html += "</div></body></html>";
  return html;
}

void handleSave() {
  if (server.hasArg("c")) { long rgb = strtol(server.arg("c").substring(1).c_str(), NULL, 16); conf.r = (rgb >> 16) & 0xFF; conf.g = (rgb >> 8) & 0xFF; conf.b = rgb & 0xFF; }
  if (server.hasArg("am")) conf.appMode = server.arg("am").toInt();
  if (server.hasArg("m"))  conf.visualMode = server.arg("m").toInt();
  if (server.hasArg("sp")) conf.speed = server.arg("sp").toInt();
  if (server.hasArg("bd")) conf.brightDay = server.arg("bd").toInt();
  if (server.hasArg("bn")) conf.brightNight = server.arg("bn").toInt();
  if (server.hasArg("ns")) conf.nightStart = server.arg("ns").toInt();
  if (server.hasArg("ne")) conf.nightEnd = server.arg("ne").toInt();
  if (server.hasArg("tv")) conf.timerVal = server.arg("tv").toInt();
  if (server.hasArg("bl")) conf.blanking = (server.arg("bl") == "1");
  server.send(200, "text/plain", "OK");
}

void handleCtrl() {
  if(conf.appMode == 1 || conf.appMode == 2) { 
    if(server.arg("a")=="start") { swRunning = true; swStart = millis() - swElapsed; }
    else if(server.arg("a")=="stop") swRunning = false;
    else if(server.arg("a")=="reset") { swRunning = false; swElapsed = 0; }
  } else if(conf.appMode >= 5) { 
    if(server.arg("a")=="start") { timerRunning = true; timerFinished = false; timerStart = millis(); timerRemaining = (conf.appMode==5) ? conf.timerVal*1000 : conf.timerVal*60000; }
    else if(server.arg("a")=="stop") timerRunning = false;
    else if(server.arg("a")=="reset") { timerRunning = false; timerFinished = false; timerRemaining = 0; }
  }
  server.sendHeader("Location", "/"); server.send(303);
}

uint32_t getEffectColor(int i, bool isNight) {
  if (isNight) return strip.Color(50, 0, 0); 
  float ph = millis() / (2500.0 - conf.speed * 20.0);
  switch (conf.visualMode) {
    case 1: return strip.ColorHSV(effectHue);
    case 2: return strip.ColorHSV(effectHue + i * 1500);
    case 3: { float s = (sin(ph * PI) + 1) / 2.0; return strip.Color(180+(75*s), 20+(40*s), 0); }
    case 4: { float s = (sin(ph * 0.8 * PI) + 1) / 2.0; return strip.Color(50+(100*s), 0, 150+(105*s)); }
    case 5: { float s = (sin(ph * PI) + 1) / 2.0; float v = 0.1+(0.9*s); return strip.Color(conf.r*v, conf.g*v, conf.b*v); }
    default: return strip.Color(conf.r, conf.g, conf.b);
  }
}

void drawDigit(int offset, int num, bool night, bool skip) {
  if(skip) return; 
  byte m = (num >= 0 && num <= 9) ? digitMap[num] : 0;
  for (int i=0; i<7; i++) if (bitRead(m, i)) strip.setPixelColor(offset+i, getEffectColor(offset+i, night));
}

void updateDisplay() {
  time_t n = time(nullptr); struct tm* t = localtime(&n);
  bool night = (t->tm_hour >= conf.nightStart || t->tm_hour < conf.nightEnd);
  strip.setBrightness(map(night ? conf.brightNight : conf.brightDay, 0, 100, 0, 255));
  strip.clear();
  uint32_t dotC = getEffectColor(14, night);
  bool dotO = (millis() % 1000 < 500);

  if (conf.appMode == 0) { // Clock
    int h = t->tm_hour, m = t->tm_min;
    drawDigit(0, h/10, night, conf.blanking && h<10); drawDigit(7, h%10, night, false);
    if(dotO) { strip.setPixelColor(14, dotC); strip.setPixelColor(15, dotC); }
    drawDigit(16, m/10, night, false); drawDigit(23, m%10, night, false);
  } else if (conf.appMode == 1 || conf.appMode == 2) { // SW
    if(swRunning) swElapsed = millis() - swStart;
    int d1, d2, d3, d4;
    if(conf.appMode == 1) { d1=((swElapsed/1000)%60)/10; d2=((swElapsed/1000)%60)%10; d3=((swElapsed/10)%100)/10; d4=((swElapsed/10)%100)%10; }
    else { d1=((swElapsed/60000)%100)/10; d2=((swElapsed/60000)%100)%10; d3=((swElapsed/1000)%60)/10; d4=((swElapsed/1000)%60)%10; }
    drawDigit(0, d1, false, conf.blanking && d1==0); drawDigit(7, d2, false, conf.blanking && d1==0 && d2==0);
    if(dotO || !swRunning) { strip.setPixelColor(14, dotC); strip.setPixelColor(15, dotC); }
    drawDigit(16, d3, false, false); drawDigit(23, d4, false, false);
  } else if (conf.appMode == 3) { // Snake
    for(int i=0; i<5; i++) strip.setPixelColor((snakePos+i)%NUMPIXELS, getEffectColor(snakePos, false));
    snakePos = (snakePos+1)%NUMPIXELS;
  } else if (conf.appMode == 4) { // Strobo
    stroboState = !stroboState;
    if(stroboState) for(int i=0;i<NUMPIXELS;i++) strip.setPixelColor(i, getEffectColor(0, false)); 
  } else if (conf.appMode >= 5) { // Timer
    if(timerRunning) { 
      unsigned long rem = (timerRemaining-(millis()-timerStart))/1000;
      if(millis()-timerStart < timerRemaining) { 
        int t1=(rem/60)/10, t2=(rem/60)%10, t3=(rem%60)/10, t4=(rem%60)%10;
        drawDigit(0, t1, false, conf.blanking && t1==0); drawDigit(7, t2, false, conf.blanking && t1==0 && t2==0); 
        if(dotO) { strip.setPixelColor(14, dotC); strip.setPixelColor(15, dotC); }
        drawDigit(16, t3, false, false); drawDigit(23, t4, false, false); 
      } else { timerFinished = true; timerRunning = false; }
    } else if (timerFinished) { if(millis()%400<200) for(int i=0;i<NUMPIXELS;i++) strip.setPixelColor(i,255,0,0); }
    else { 
      int v1=(conf.appMode==5)?0:conf.timerVal/10, v2=(conf.appMode==5)?0:conf.timerVal%10;
      int v3=(conf.appMode==5)?conf.timerVal/10:0, v4=(conf.appMode==5)?conf.timerVal%10:0;
      drawDigit(0,v1,false,conf.blanking && v1==0); drawDigit(7,v2,false,conf.blanking && v1==0 && v2==0); 
      drawDigit(16,v3,false,false); drawDigit(23,v4,false,false); 
    }
  }
  strip.show(); effectHue += map(conf.speed, 1, 100, 50, 1500);
}

void setup() {
  strip.begin(); WiFi.begin(wifiname, wifipassword);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  configTime(MY_TZ, "pool.ntp.org");
  server.on("/", [](){ server.send(200, "text/html", getHTML()); });
  server.on("/save", handleSave); server.on("/ctrl", handleCtrl);
  server.begin();
}

void loop() {
  server.handleClient();
  int d = (conf.appMode == 4)?map(101-conf.speed,1,100,20,200):40;
  if(millis()-lastUpdate >= d) { lastUpdate = millis(); updateDisplay(); }
}