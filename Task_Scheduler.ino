#include "WiFiS3.h"
#include <Servo.h>
#include "arduino_secrets.h"
#include <TaskScheduler.h>

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;        // your network password (use for WPA, or use as key for WEP)

int led = LED_BUILTIN;
int status = WL_IDLE_STATUS;
WiFiServer server(80);

Servo myservo;  // 서보 객체 생성
int servoPin = 9; // 서보가 연결된 핀

Scheduler runner;

void printWifiStatus() {
  // 연결된 네트워크의 SSID를 출력:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // 보드의 IP 주소를 출력:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // 수신 신호 강도를 출력:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // 브라우저에서 접속할 주소를 출력:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

void handleClient() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("<p style=\"font-size:7vw;\">Click <a href=\"/H\">here</a> to turn the LED on<br></p>");
            client.print("<p style=\"font-size:7vw;\">Click <a href=\"/L\">here</a> to turn the LED off<br></p>");
            client.print("<p style=\"font-size:7vw;\">Click <a href=\"/rotate\">here</a> to rotate the motor<br></p>");
            
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // 새 줄 문자를 받은 경우, currentLine을 지웁니다.
            currentLine = "";
          }
        } else if (c != '\r') {  // 캐리지 리턴 문자가 아닌 다른 것을 받으면,
          currentLine += c;      // currentLine 끝에 추가
        }

        // 클라이언트 요청이 "GET /H" 또는 "GET /L" 또는 "GET /rotate"인지 확인:
        if (currentLine.endsWith("GET /H")) {
          digitalWrite(LED_BUILTIN, HIGH);               // GET /H LED를 켭니다.
        }
        if (currentLine.endsWith("GET /L")) {
          digitalWrite(LED_BUILTIN, LOW);                // GET /L LED를 끕니다.
        }
        if (currentLine.endsWith("GET /rotate")) {
          myservo.write(35);  // 서보를 35도 회전
          delay(1000);         // 서보가 위치에 도달할 때까지 대기
          myservo.write(0);    // 서보를 0도로 복귀
        }
      }
    }
    // 연결을 종료:
    client.stop();
    Serial.println("client disconnected");
  }
}

Task task1(1000, TASK_FOREVER, &handleClient); // 1초 주기

void setup() {
  Serial.begin(9600);      // 시리얼 통신 초기화
  pinMode(led, OUTPUT);      // LED 핀 모드 설정
  myservo.attach(servoPin);  // 서보 객체를 핀 9에 연결

  // WiFi 모듈 확인:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // 계속하지 않음
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // WiFi 네트워크에 연결 시도:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // 네트워크 이름(SSID) 출력

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status

  runner.init();
  runner.addTask(task1);
  task1.enable();
}

void loop() {
  runner.execute();
}
