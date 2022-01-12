import pyqrcode
import os

import gspread
from oauth2client.service_account import ServiceAccountCredentials

scope = ["https://spreadsheets.google.com/feeds",'https://www.googleapis.com/auth/spreadsheets',"https://www.googleapis.com/auth/drive.file","https://www.googleapis.com/auth/drive"]

creds = ServiceAccountCredentials.from_json_keyfile_name("keys.json", scope)

client = gspread.authorize(creds)

sh = client.open_by_key("14os1B8xQeLLX0_eWIHMfjnH1KNUHYJmv53jVGGxcDHo")

lishe = sh.worksheets()

os.chdir(os.getcwd()+'/NTMK')

for i in lishe :
  if i.title not in os.listdir():
    os.mkdir(i.title)
  else:
    continue
  
for i in lishe:
  for a in range (1,int(i.col_values(1)[-1])+1):
    s= i.title + ", " + str(a)
  
    url = pyqrcode.create(s)

    url.png(f'{i.title}/{a}.png', scale = 6)