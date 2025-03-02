#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

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
        <p id="startMessage" style="
            position: absolute; 
            top: 50%; left: 50%;
            transform: translate(-50%, -50%);
            font-size: 24px; 
            font-weight: bold;
            color: white;
        ">Press Switch button to start</p>
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
    const scoreDisplay = document.getElementById("score");
    const timerDisplay = document.getElementById("timer");
    const countdownDisplay = document.getElementById("countdown");
    const startMessage = document.getElementById("startMessage");

    let snake = [{ x: 200, y: 200 }];

    const generateFoodPosition = () => {
        let newFood;
        let isOnSnake;
        do {
            newFood = {
                x: Math.floor(Math.random() * 20) * 20,
                y: Math.floor(Math.random() * 20) * 20
            };
            isOnSnake = snake.some(segment => segment.x === newFood.x && segment.y === newFood.y);
        } while (isOnSnake);
        return newFood;
    };
    let food = generateFoodPosition();

    let score = 0;
    let timeElapsed = 0;
    let gameRunning = false;
    let timerInterval;
    let gameSpeed = 200;

    let direction = "RIGHT";
    let nextDirection = "RIGHT";
    let socket = new WebSocket("ws://" + location.host + ":81/");
    socket.onmessage = function(event) {
        let message = event.data;
        if (message === "START") {
            startGame();
        } else {
            let newDirection = message;
            if ((direction === "RIGHT" && newDirection !== "LEFT") ||
                (direction === "LEFT" && newDirection !== "RIGHT") ||
                (direction === "UP" && newDirection !== "DOWN") ||
                (direction === "DOWN" && newDirection !== "UP")) {
                nextDirection = newDirection;
            }
        }
    };

    const startGame = () => {
        score = 0;
        timeElapsed = 0;
        scoreDisplay.innerText = "Score: 0";
        timerDisplay.innerText = "Time: 0s";
        snake = [{ x: 200, y: 200 }];
        direction = "RIGHT";
        nextDirection = "RIGHT";
        gameRunning = false;
        gameSpeed = 200;

        startMessage.style.display = "none"; // ซ่อนข้อความเริ่มต้น
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

    const updateGame = () => {
        if (!gameRunning) return;

        direction = nextDirection;

        let head = { ...snake[0] };
        if (direction === "LEFT") head.x -= 20;
        if (direction === "RIGHT") head.x += 20;
        if (direction === "UP") head.y -= 20;
        if (direction === "DOWN") head.y += 20;

        if (head.x < 0) head.x = canvas.width - 20;
        else if (head.x >= canvas.width) head.x = 0;
        if (head.y < 0) head.y = canvas.height - 20;
        else if (head.y >= canvas.height) head.y = 0;

        for (let i = 1; i < snake.length; i++) {
            if (head.x === snake[i].x && head.y === snake[i].y) {
                gameRunning = false;
                clearInterval(timerInterval);
                drawGame();
                setTimeout(() => {
                    timerDisplay.innerText = `💀 Game Over! Time: ${timeElapsed}s`;
                    startMessage.style.display = "block"; // แสดงข้อความเริ่มต้นอีกครั้ง
                }, 100);
                return;
            }
        }

        let foodEaten = head.x === food.x && head.y === food.y;

        snake.unshift(head);
        if (!foodEaten) {
            snake.pop();
        } else {
            score += 1;
            scoreDisplay.innerText = "Score: " + score;
            food = generateFoodPosition();
            
            if (score === 20) {
                gameRunning = false;
                clearInterval(timerInterval);
                drawGame();
                timerDisplay.innerText = `🎉 You Win! 🎉 Time: ${timeElapsed}s`;
                startMessage.style.display = "block";
                return;
            }

            // จงเขียนโค้ดเพื่อกำหนดค่า gameSpeed ตาม score ดังต่อไปนี้ 
            // score ≤ 10 → gameSpeed = 200, 
            // score ≤ 15 → gameSpeed = 170, 
            // score ≤ 20 → gameSpeed = 150




        }

        drawGame();
        setTimeout(updateGame, gameSpeed);
    };

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
</script>
</body>
</html>
)rawliteral";

// WiFi AP Config
const char* ssid = "Your-Wifi-Name"; // ตั้งเองเลย
const char* password = "Your-Wifi-Password"; // ตั้งเองเลย

WebServer server(80);
WebSocketsServer webSocket(81);

const int xPin = 34;  // VRX pin
const int yPin = 35;  // VRY pin
const int swPin = 32; // SW pin
const int deadzone = 20;

String lastDirection = "RIGHT";

void setup() {
  Serial.begin(115200);
  Serial.println("Setup started");

  pinMode(swPin, INPUT_PULLUP); // ตั้งค่า SW เป็น INPUT_PULLUP

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", []() {
    Serial.println("Serving HTML page");
    server.send_P(200, "text/html", htmlPage);
  });
  server.begin();
  Serial.println("Web server started");
  
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSocket server started");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_CONNECTED) {
    Serial.println("WebSocket Connected");
  }
}

void loop() {
  server.handleClient();
  webSocket.loop();

  static unsigned long lastSentTime = 0;
  const unsigned long sendInterval = 150;

  // อ่านค่า SW (ปุ่มจอยสติ๊ก)
  static int lastButtonState = HIGH;
  int buttonState = digitalRead(swPin);
  if (lastButtonState == HIGH && buttonState == LOW) { // ตรวจจับการกดปุ่ม (ขอบลง)
    webSocket.broadcastTXT("START");
    lastDirection = "RIGHT"; // รีเซ็ต lastDirection เป็น RIGHT เมื่อเริ่มเกมใหม่
    Serial.println("Start button pressed, lastDirection reset to RIGHT");
  }
  lastButtonState = buttonState;

  // อ่านค่าแกน X และ Y
  int xSum = 0, ySum = 0;
  const int samples = 5;
  for (int i = 0; i < samples; i++) {
    xSum += analogRead(xPin);
    ySum += analogRead(yPin);
    delay(2);
  }
  
  int xValue = xSum / samples;
  int yValue = ySum / samples;

  int vx = map(xValue, 0, 4095, -100, 100);
  int vy = map(yValue, 0, 4095, -100, 100);

  if (abs(vx) < deadzone) vx = 0;
  if (abs(vy) < deadzone) vy = 0;

  String newDirection = lastDirection;

  if (abs(vx) > abs(vy) && abs(vx) >= 50) {
    newDirection = (vx > 0) ? "RIGHT" : "LEFT";
  } else if (abs(vy) >= 50) {
    newDirection = (vy > 0) ? "DOWN" : "UP";
  }

  unsigned long currentTime = millis();
  if (!((lastDirection == "RIGHT" && newDirection == "LEFT") ||
        (lastDirection == "LEFT" && newDirection == "RIGHT") ||
        (lastDirection == "UP" && newDirection == "DOWN") ||
        (lastDirection == "DOWN" && newDirection == "UP"))) {
    
    if (newDirection != lastDirection && (currentTime - lastSentTime >= sendInterval)) { 
      lastDirection = newDirection;
      String message = newDirection;
      webSocket.broadcastTXT(message);
      lastSentTime = currentTime;
      Serial.print("Direction sent: ");
      Serial.println(newDirection);
    }
  }

  Serial.print("vx: ");
  Serial.print(vx);
  Serial.print(" | vy: ");
  Serial.println(vy);

  delay(50);
}