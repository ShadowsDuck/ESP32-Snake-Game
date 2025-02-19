#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ประกาศ prototype ก่อน setup()
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

// HTML + JavaScript (ต้องอยู่ก่อน setup())
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Snake Game</title>
    <style>
        body { text-align: center; font-family: Arial, sans-serif; }
        canvas { background: black; display: block; margin: auto; }
        #score, #timer { font-size: 20px; margin: 10px; }
    </style>
</head>
<body>
    <h1>Snake Game</h1>
    <p id="score">Score: 0</p>
    <p id="timer">Time: 0s</p>
    
    <div style="position: relative; display: inline-block;">
        <canvas id="gameCanvas" width="400" height="400"></canvas>
        <button id="startButton" style="
            position: absolute; 
            top: 50%; left: 50%;
            transform: translate(-50%, -50%);
            padding: 10px 20px; 
            font-size: 18px;
            cursor: pointer;
        ">Start</button>
        <p id="countdown" style="
            position: absolute; 
            top: 50%; left: 50%;
            transform: translate(-50%, -50%);
            font-size: 40px; 
            font-weight: bold;
            color: white; 
            display: none;
        "></p>
    </div>

<script>
    const canvas = document.getElementById("gameCanvas");
    const ctx = canvas.getContext("2d");
    const startButton = document.getElementById("startButton");
    const scoreDisplay = document.getElementById("score");
    const timerDisplay = document.getElementById("timer");
    const countdownDisplay = document.getElementById("countdown");

    let snake = [{ x: 200, y: 200 }];
    let direction = "RIGHT";
    let food = { x: Math.floor(Math.random() * 20) * 20, y: Math.floor(Math.random() * 20) * 20 };
    let score = 0;
    let timeElapsed = 0;
    let gameRunning = false;
    let timerInterval;
    let gameSpeed = 150; // ความเร็วเกมเริ่มต้น

    let socket = new WebSocket("ws://" + location.host + ":81/");
    socket.onmessage = function(event) {
        let [vx, vy] = event.data.split(",").map(Number);
        if (vx < -50) direction = "LEFT";
        else if (vx > 50) direction = "RIGHT";
        if (vy < -50) direction = "UP";
        else if (vy > 50) direction = "DOWN";
    };

    // ฟังก์ชันเริ่มเกม รีเซ็ตค่าต่าง ๆ และเริ่มการนับถอยหลังก่อนเริ่มเล่น
    const startGame = () => {
        startButton.style.display = "none";
        score = 0;
        timeElapsed = 0;
        scoreDisplay.innerText = "Score: 0";
        timerDisplay.innerText = "Time: 0s";
        snake = [{ x: 200, y: 200 }];
        direction = "RIGHT";
        gameRunning = false;
        gameSpeed = 150;

        countdownDisplay.style.display = "block";
        countdownDisplay.innerText = "3";
        setTimeout(() => countdownDisplay.innerText = "2", 1000);
        setTimeout(() => countdownDisplay.innerText = "1", 2000);
        setTimeout(() => {
            countdownDisplay.innerText = "Go!";
            setTimeout(() => countdownDisplay.style.display = "none", 500);
            
            gameRunning = true;
            timerInterval = setInterval(() => {
                timeElapsed++;
                timerDisplay.innerText = "Time: " + timeElapsed + "s";
            }, 1000);
            updateGame();
        }, 3000);
    };

    // ฟังก์ชันหลักสำหรับอัปเดตสถานะของเกม เช่น การเคลื่อนที่ของงูและการตรวจสอบการกินอาหาร
    const updateGame = () => {
        if (!gameRunning) return;

        let head = { ...snake[0] };
        if (direction === "LEFT") head.x -= 20;
        if (direction === "RIGHT") head.x += 20;
        if (direction === "UP") head.y -= 20;
        if (direction === "DOWN") head.y += 20;

        if (head.x < 0) head.x = canvas.width - 20;
        else if (head.x >= canvas.width) head.x = 0;
        if (head.y < 0) head.y = canvas.height - 20;
        else if (head.y >= canvas.height) head.y = 0;

        let foodEaten = head.x === food.x && head.y === food.y;

        snake.unshift(head);
        if (!foodEaten) {
            snake.pop();
        } else {
            score += 1;
            scoreDisplay.innerText = "Score: " + score;
            food = { x: Math.floor(Math.random() * 20) * 20, y: Math.floor(Math.random() * 20) * 20 };
            
            // เงื่อนไขเมื่อคะแนนถึง 30 เพื่อให้เกมหยุดหลังจากงูกินอาหารสุดท้าย
            if (score === 30) {
                drawGame();
                setTimeout(() => {
                    gameRunning = false;
                    clearInterval(timerInterval);
                    timerDisplay.innerText = `🎉 You Win! 🎉 Time: ${timeElapsed}s`;
                    startButton.style.display = "block";
                }, 100);
                return;
            }

            // สร้างเงื่อนไข ความเร็วเกม ถ้า score <= 10 ให้ gameSpeed = 150, <= 20 gameSpeed = 120, <= 30 gameSpeed = 100
            // CODE HERE...
        }

        drawGame();
        setTimeout(updateGame, gameSpeed);
    };

    // ฟังก์ชันวาดกราฟิกของเกมบน canvas รวมถึงตัวงูและอาหาร
    const drawGame = () => {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.fillStyle = "red";
        ctx.fillRect(food.x, food.y, 20, 20);

        ctx.fillStyle = "lime";
        for (let i = 1; i < snake.length; i++) {
            ctx.fillRect(snake[i].x, snake[i].y, 20, 20);
        }

        ctx.fillStyle = "yellow";
        ctx.fillRect(snake[0].x, snake[0].y, 20, 20);
    };

    startButton.addEventListener("click", startGame);
</script>
</body>
</html>
)rawliteral";

// WiFi AP Config
const char* ssid = "SnakeGame-ESP32";
const char* password = "12345678";

WebServer server(80);
WebSocketsServer webSocket(81);

const int xPin = 34;  // VRX pin
const int yPin = 35;  // VRY pin

void setup() {
  Serial.begin(115200);

  // ตั้งค่า WiFi เป็น Access Point
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP Started!");

  // ตั้งค่า Web Server
  server.on("/", []() {
    server.send_P(200, "text/html", htmlPage);
  });
  server.begin();
  
  // ตั้งค่า WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

// Handle WebSocket Events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.println("WebSocket Connected");
  }
}

void loop() {
  server.handleClient();
  webSocket.loop();

  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);

  // Map ค่าจาก 0-4095 ให้เป็นช่วง -100 ถึง 100
  int vx = map(xValue, 0, 4095, -100, 100);
  int vy = map(yValue, 0, 4095, -100, 100);

  // Debug ค่า vx, vy บน Serial Monitor
  Serial.print("vx: ");
  Serial.print(vx);
  Serial.print(" | vy: ");
  Serial.println(vy);

  // ส่งค่าไปที่ WebSocket
  String message = String(vx) + "," + String(vy);
  webSocket.broadcastTXT(message);

  delay(100);  // ควบคุมอัตราการอัปเดต
}
