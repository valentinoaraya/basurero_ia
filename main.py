from keras.models import load_model
import cv2
import numpy as np
import time
import serial

arduino = serial.Serial("/dev/cu.usbserial-10", 9600)
time.sleep(2)

np.set_printoptions(suppress=True)

model = load_model("model/keras_model.h5", compile=False)
class_names = open("model/labels.txt", "r").readlines()

camera = cv2.VideoCapture(0)

last_detection_time = 0
cooldown = 5

while True:
    ret, image = camera.read()

    image = cv2.resize(image, (224, 224), interpolation=cv2.INTER_AREA)

    cv2.imshow("Detector de basura", image)

    image = np.asarray(image, dtype=np.float32).reshape(1, 224, 224, 3)

    image = (image / 127.5) - 1

    prediction = model.predict(image)
    index = np.argmax(prediction)
    class_name = class_names[index]
    confidence_score = prediction[0][index]

    print("Class:", class_name[2:], end="")
    print("Confidence Score:", str(np.round(confidence_score * 100))[:-2], "%")

    current_time = time.time()

    if confidence_score > 0.90:

        if current_time - last_detection_time > cooldown:

            if "PLASTICO" in class_name:

                print("Detectado: PLASTICO")

                arduino.write(b'P')

                last_detection_time = current_time

            elif "PAPEL" in class_name:

                print("Detectado: PAPEL")

                arduino.write(b'L')

                last_detection_time = current_time

            elif "METAL" in class_name:

                print("Detectado: METAL")

                arduino.write(b'M')

                last_detection_time = current_time

    keyboard_input = cv2.waitKey(1)

    if keyboard_input == 27 or keyboard_input == 113:
        break

camera.release()
cv2.destroyAllWindows()