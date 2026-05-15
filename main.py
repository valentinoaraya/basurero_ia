from typing import Any


from keras.models import load_model
import cv2
import numpy as np
import time
import serial
from collections import Counter

arduino = serial.Serial("/dev/cu.usbserial-10", 9600)
time.sleep(2)

np.set_printoptions(suppress=True)

model = load_model("model/keras_model.h5", compile=False)
class_names = open("model/labels.txt", "r").readlines()

camera = cv2.VideoCapture(0)

last_detection_time = 0
cooldown = 5

print("Esperando START desde Arduino...")

while True:
    message = arduino.readline().decode().strip()

    print("Mensaje:", message)

    if message == "QUIT":
        break

    if message == "START":
        print("Iniciando clasificación...")

        predictions_list = []

        start_time = time.time()

        while time.time() - start_time < 3:
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


            if confidence_score > 0.90:

                predictions_list.append(class_name)
                print(
                        class_name,
                        round(confidence_score * 100, 2)
                    )

            cv2.waitKey(1)

        if len(predictions_list) > 0:
            final_prediction = Counter[str](
                predictions_list
            ).most_common(1)[0][0]
            print("Resultado final:", final_prediction)

            if final_prediction.split(" ")[1].strip() == "PLASTICO":
                to_send_arduino = 'P'
            elif final_prediction.split(" ")[1].strip() == "PAPEL":
                to_send_arduino = 'L'
            elif final_prediction.split(" ")[1].strip() == "METAL":
                to_send_arduino = 'M'
            else:
                to_send_arduino = ""

            print(to_send_arduino)

            if to_send_arduino != "":
                arduino.write(to_send_arduino.encode())
   
        else:
            print("No se detectó nada")


    keyboard_input = cv2.waitKey(1)

    if keyboard_input == 27 or keyboard_input == 113:
        break

camera.release()
cv2.destroyAllWindows()
arduino.close()