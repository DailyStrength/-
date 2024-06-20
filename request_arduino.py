import cv2
import numpy as np
import requests

# 웹캠 신호 받기
VideoSignal = cv2.VideoCapture(0) # 기본 웹캠을 사용하여 비디오 캡처 객체 생성

# YOLO 가중치 파일과 CFG 파일 로드
YOLO_net = cv2.dnn.readNet("yolov3-tiny.weights", "yolov3-tiny.cfg")# YOLO 네트워크를 구성하는 가중치와 설정 파일 로드

# YOLO NETWORK 재구성
classes = []
with open("coco.names", "r", encoding="UTF-8") as f:
    classes = [line.strip() for line in f.readlines()] # 객체 이름이 저장된 파일을 읽어 리스트에 저장

layer_names = YOLO_net.getLayerNames()# YOLO 네트워크의 모든 레이어 이름을 가져옴
output_layers = [layer_names[i - 1] for i in YOLO_net.getUnconnectedOutLayers()]# 출력 레이어의 이름을 가져옴

arduino_ip = "192.168.91.223"  # 아두이노의 IP 주소
# 아두이노로 명령을 보내는 함수
def send_command(command):
    url = f"http://{arduino_ip}/{command}"# 명령을 포함한 URL 생성
    response = requests.get(url)# 해당 URL로 GET 요청 전송
    if response.status_code == 200:
        print(f"Successfully sent {command} command.")# 요청이 성공하면 메시지 출력
    else:
        print(f"Failed to send {command} command. Status code: {response.status_code}")

while True:
    # 웹캠 프레임 읽기
    ret, frame = VideoSignal.read()# 비디오 캡처 객체로부터 프레임 읽기
    if not ret:
        print("Failed to grab frame")# 프레임 읽기에 실패하면 메시지 출력
        break
    h, w, _ = frame.shape# 프레임의 높이와 너비 가져오기

    # YOLO 입력
    blob = cv2.dnn.blobFromImage(frame, 0.00392, (416, 416), (0, 0, 0), True, crop=False)
    YOLO_net.setInput(blob)# YOLO 네트워크에 블롭 입력
    outs = YOLO_net.forward(output_layers)# YOLO 네트워크에서 추론 실행

    class_ids = []# 탐지된 객체의 클래스 ID를 저장할 리스트
    confidences = []# 탐지된 객체의 신뢰도를 저장할 리스트
    boxes = []# 탐지된 객체의 경계 상자를 저장할 리스트

    for out in outs:
        for detection in out:
            scores = detection[5:]# 탐지된 객체의 클래스별 점수
            class_id = np.argmax(scores) # 가장 높은 점수를 받은 클래스 ID
            confidence = scores[class_id]# 해당 클래스의 신뢰도

            if confidence > 0.5:
                # 객체 탐지됨
                center_x = int(detection[0] * w)# 탐지된 객체의 중심 x 좌표
                center_y = int(detection[1] * h)
                dw = int(detection[2] * w)
                dh = int(detection[3] * h)

                 # 사각형 좌표
                x = int(center_x - dw / 2)
                y = int(center_y - dh / 2)
                boxes.append([x, y, dw, dh])
                confidences.append(float(confidence))# 신뢰도 리스트에 추가
                class_ids.append(class_id)# 클래스 ID 리스트에 추가

    indexes = cv2.dnn.NMSBoxes(boxes, confidences, 0.45, 0.4)# 비최대 억제를 사용하여 경계 상자 필터링

    detected_person = False# "cat" 객체가 탐지되었는지 여부를 저장할 변수

    for i in range(len(boxes)):
        if i in indexes:
            x, y, w, h = boxes[i]
            label = str(classes[class_ids[i]])# 탐지된 객체의 클래스 이름
            score = confidences[i]# 탐지된 객체의 신뢰도

            if label == "cat":
                detected_person = True# "cat" 객체가 탐지되었음을 표시

            # Draw bounding box and class information on the image
            cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 0, 255), 2)
            cv2.putText(frame, f"{label} {score:.2f}", (x, y - 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 2)

    if detected_person:
        send_command("rotate")# "cat" 객체가 탐지되면 아두이노에 "rotate" 명령 전송

    cv2.imshow("YOLOv3 Tiny", frame) # 프레임을 화면에 표시

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# 모든 작업이 완료되면 비디오 캡처 객체와 모든 윈도우 해제
VideoSignal.release()
cv2.destroyAllWindows()
