#khai báo thư viện
from keras.models import Sequential,load_model
from keras.preprocessing import image

import gspread
from oauth2client.service_account import ServiceAccountCredentials

import datetime
import pytz

import cv2
import numpy as np
import pyzbar.pyzbar as pyzbar

import os
import requests

import board
import busio as io
import adafruit_mlx90614

from playsound import playsound

#tạo layout trên spreadsheet cho một ngày mới

scope = ["https://spreadsheets.google.com/feeds",'https://www.googleapis.com/auth/spreadsheets',"https://www.googleapis.com/auth/drive.file","https://www.googleapis.com/auth/drive"]

creds = ServiceAccountCredentials.from_json_keyfile_name("keys.json", scope)

client = gspread.authorize(creds)

sh = client.open_by_key("14os1B8xQeLLX0_eWIHMfjnH1KNUHYJmv53jVGGxcDHo")  # Open the spreadhseet

datvn2 = str(pytz.utc.localize(datetime.datetime.now()))
a = datvn2[0:10]

#lấy file âm thanh
granted = os.getcwd() + "/granted.wav"
denied = os.getcwd() + "/denied.wav"

#setup cho cảm biến nhiệt
i2c = io.I2C(board.SCL, board.SDA, frequency=100000)
mlx = adafruit_mlx90614.MLX90614(i2c)

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3'

lishe = sh.worksheets()

for i in lishe:

  l = len(i.row_values(3))

  ce = i.cell(1,l-3).value

  if a == ce:
    continue

  if a != ce:
    i.insert_cols([[a,"","Điểm danh"],["","Tình trạng sức khỏe","Nhiệt độ"],["","","Sốt/Ho/Cảm"],["","","Khác"]],col=l+1,value_input_option="USER_ENTERED")

    i.delete_columns(l+5,i.col_count)

    i.merge_cells(name=gspread.utils.rowcol_to_a1(1, l+1)+':'+gspread.utils.rowcol_to_a1(1, l+4))

    i.merge_cells(name=gspread.utils.rowcol_to_a1(2, l+2)+':'+gspread.utils.rowcol_to_a1(2, l+4))

    i.format('A1:'+gspread.utils.rowcol_to_a1(3, l+4),{
      "backgroundColor": {
        "red": 0.8, 
        "green":  0,
        "blue": 0.568
      },
      "horizontalAlignment": "CENTER",
      "verticalAlignment":"MIDDLE",
      "textFormat": {
        "foregroundColor": {
          "red": 0.960, 
          "green":  0.968,
          "blue": 0.211
        },
        "fontSize": 12,
        "bold": True
      },
      "borders":{
        "top": {
          "style": "DOTTED",
          "width": 2,
          "color": {
            "red": 0.960, 
            "green":  0.968,
            "blue": 0.211
          }        
        },
        "bottom": {
          "style": "DOTTED",
          "width": 1,
          "color": {
            "red": 0.960, 
            "green":  0.968,
            "blue": 0.211
          }
        },
        "left": {
          "style": "DOTTED",
          "width": 1,
          "color": {
            "red": 0.960, 
            "green":  0.968,
            "blue": 0.211
          }
        },
        "right": {
          "style": "DOTTED",
          "width": 1,
          "color": {
            "red": 0.960, 
            "green":  0.968,
            "blue": 0.211
          }
        }
      }
  })

#nhận diện khuôn mặt + khẩu trang

cap = cv2.VideoCapture(0)
face_cascade = cv2.CascadeClassifier('haarcascade_frontalface_default.xml')
font = cv2.FONT_HERSHEY_PLAIN
model = load_model("model_face_mask.h5")

while True: 
    _, img = cap.read() 
    f = cv2.flip(img,1)
    face = face_cascade.detectMultiScale(f, scaleFactor=1.1, minNeighbors=4)
    for (x, y, w, h) in face:
        #sử dụng model đã train
        face_img = f[y:y + h, x:x + w]
        cv2.imwrite('temp.jpg', face_img)
        test_image = image.load_img('temp.jpg', target_size=(150, 150, 3))
        test_image = image.img_to_array(test_image)
        test_image = np.expand_dims(test_image, axis=0)
        pred = model.predict_classes(test_image)[0][0]

        color = (0, 0, 255) #BGR
        stroke = 2
        height = y + h #height
        width = x + w  #width
        rec_w = (x+width)//2
        rec_h = (y+height)//2

        if pred == 1:
            #không có khẩu trang -> báo còi + thể hiện trên màn hình
            playsound(denied)
            cv2.rectangle(f, (x, y), (x + w, y + h), (0, 0, 255), 3)
            cv2.putText(f, 'UNMASKED', ((x + w) // 2, y + h + 20), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 3)
            cv2.line(f, (rec_w,0), (rec_w,rec_h*2), (0, 0, 255), 3) #rec_linex
            cv2.line(f, (0,rec_h), (rec_w*2,rec_h), (0, 0, 255), 3) #rec_liney
        else:
            #có khẩu trang -> thể hiện trên màng hình
            cv2.rectangle(f, (x, y), (x + w, y + h), (0, 255, 0), 3)
            cv2.putText(f, 'MASKED', ((x + w) // 2, y + h + 20), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 3)
            cv2.line(f, (rec_w,0), (rec_w,rec_h*2), (0, 255, 0), 3) #rec_linex
            cv2.line(f, (0,rec_h), (rec_w*2,rec_h), (0, 255, 0), 3) #rec_liney
        #thể hiện thời gian (ngày tháng giờ phút)
        datet = str(datetime.datetime.now())
        cv2.putText(f, datet, (400, 450), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)

    #giao tiếp với mạch ESP qua Firebase
    if (rec_h <= 255) and (rec_h >= 225):
       dataString = "{:.d}".format(100)
       dataSend = int(ambientString)
       data = {
       "data": dataSend,
       }
       db.child("test").child("data").set(data)
    elif (rec_h > 255):
       dataString = "{:.d}".format(110)
       dataSend = int(ambientString)
       data = {
       "data": dataSend,
       }
       db.child("test").child("data").set(data)
    elif (rec_h < 225):
       dataString = "{:.d}".format(101)
       dataSend = int(ambientString)
       data = {
       "data": dataSend,
       }
       db.child("test").child("data").set(data)
    else:
       dataString = "{:.d}".format(111)
       dataSend = int(ambientString)
       data = {
       "data": dataSend,
       }
       db.child("test").child("data").set(data)

    #đo nhiệt độ

    ambientTemp = "{:.2f}".format(mlx.ambient_temperature)
    targetTemp = "{:.2f}".format(mlx.object_temperature)

    #điểm danh bằng mã QR

    #cập nhật thời gian
    datvn = str(pytz.utc.localize(datetime.datetime.now()))
    a = datvn[0:10]
    b = datvn[11:19]

    decodedObjects = pyzbar.decode(f)
    #quét mã QR nếu có
    for obj in decodedObjects:
        #phát âm thanh xát nhật đã quét và hiện thị trên màng hình
        playsound(granted)
        cv2.putText(f, "Scanning Done", (50, 50), font, 2,(0, 255, 0), 3)

        inp = str(obj.data)

        sheet = sh.worksheet(inp[2:7])

        l = len(sheet.row_values(3))

        #cập nhật lên spreadsheet thông tin + tình trạng học sinh
        if str(sheet.cell(int(inp[9:-1])+3,l-3).value).startswith("Có") or str(sheet.cell(int(inp[9:-1])+3,l-3).value).startswith('Trễ'):
            sheet.update_cell(int(inp[9:-1])+3,l-2, str(tagetTemp))
            continue
        else:
            #before 6h41 <-- giờ quy định 
            if (int(b[:2]) <= 6) and (int(b[3:5]) <= 41):
                sheet.update_cell(int(inp[9:-1])+3,l-3, f"Có {b}")
            else:
                sheet.update_cell(int(inp[9:-1])+3,l-3, f"Trễ : {b}")

    #2 trục

    cv2.line(f, (320,480), (320,0), (255, 0, 0), 2) #framecenterlinex
    cv2.line(f, (0,240), (640,240), (255, 0, 0), 2) #framecenterliney

    #thể hiện tất cả trên màng hình

    cv2.imshow('main', f)

    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()