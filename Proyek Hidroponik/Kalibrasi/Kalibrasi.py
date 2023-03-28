from tkinter import *
import tkinter.ttk as ttk
import csv
from random import random
from datetime import datetime
import time
import threading
import os
import paho.mqtt.client as mqtt
import numpy as np
import serial

mode = "MQTT" #sesuaikan dengan mode operasi, "MQTT" dan "Serial" 

port = 'COM1' # port default untuk arduino 
port_num = 1 # nomor port default 
conn_status = 0 # status koneksi, akan berubah menjadi 1 ketika sudah 
                # terhubung ke arduino 

while (conn_status == 0 and port_num <= 255): # loop dari port 1 hingga port 255 
    # fungsi di dalam loop ini berfungsi untuk mendeteksi port
    # arduino yang terhubung ke laptop secara otomatis
    try:
        # coba koneksi untuk port pertama
        arduino = serial.Serial(port = port, baudrate = 9600, timeout = .1)
        conn_status = 1
        print("Connected successfully to " + port)
        print("Wait a few seconds to prepare Arduino")
        time.sleep(2)
    except:
        # jika gagal, lanjutkan ke port berikutnya
        port_num = port_num + 1
        port = 'COM' + str(port_num)
        continue


def on_connect(client, userdata, flags, rc):
    client.subscribe("hidroponik_ppm")

def on_message(client, userdata, msg):
    data = msg.payload.decode('utf-8')
    print(data)
    client.loop_stop()
    file = open("data.txt", "w")
    file.write(data)
    file.close()

def task1():
    while True:
        if (mode == "MQTT"):
            client.on_connect = on_connect
            client.on_message = on_message
            client.connect("broker.emqx.io", 1883, 60)
            client.loop_forever()

        elif (mode == "Serial"):
            data = str(arduino.readline().decode('utf-8'))
            print(data)
            client.loop_stop()
            file = open("data.txt", "w")
            file.write(data)
            file.close()

client = mqtt.Client("ria-cantik")

def tampilWindow():
    root = Tk()
    root.title("Ria Cantik...")
    width = 500
    height = 400
    screen_width = root.winfo_screenwidth()
    screen_height = root.winfo_screenheight()
    x = (screen_width/2) - (width/2)
    y = (screen_height/2) - (height/2)
    root.geometry("%dx%d+%d+%d" % (width, height, x, y))
    #root.resizable(0, 0)

    def appendData():
        try:
                file = open("data.txt", "r")
        except:
            file = open("data.txt", "w")
            file.close()
            file = open("data.txt", "r")
        file1 = open("raw_data.txt", "a")

        now = datetime.now()
        print("boo")
        analog = str(file.readlines())
        analog = analog[2:len(analog) - 2]
        tds = textDataMasuk.get(1.0, END)
        waktu = now.strftime("%m/%d/%Y - %H:%M:%S")
        tree.insert("", 0, values = (waktu, analog, tds))
        textDataMasuk.delete("1.0", END)
        
        dataToWrite = waktu + "," + str(analog) + "," + str(tds)
        file1.writelines(dataToWrite)
        file1.close()

    b1 = Button(root, text = "Input Data", command = appendData)
    b1.pack(side = BOTTOM)
    textDataMasuk = Text(root, height = 1, width = 20)
    textDataMasuk.pack(side = BOTTOM)
    labelDataMasuk = Label(root, text = "Masukkan data dari alat ukur standar")
    labelDataMasuk.pack(side = BOTTOM)

    TableMargin = Frame(root, width = 500)
    TableMargin.pack(side=TOP, fill = "both")
    scrollbarx = Scrollbar(TableMargin, orient = HORIZONTAL)
    scrollbary = Scrollbar(TableMargin, orient = VERTICAL)
    tree = ttk.Treeview(TableMargin, columns = ("Waktu", "Output Analog", "Output Alat Ukur Standar"), 
                        height=400, selectmode = "extended", yscrollcommand=scrollbary.set, 
                        xscrollcommand=scrollbarx.set) 
    
    scrollbary.config(command=tree.yview)
    scrollbary.pack(side = RIGHT, fill = Y)
    tree.heading('Waktu', text = "Waktu", anchor = W)
    tree.heading('Output Analog', text = "Output Analog", anchor = W)
    tree.heading('Output Alat Ukur Standar', text = "Output Alat Ukur Standar", anchor = W)
    tree.column('#0', stretch = NO, minwidth = 0, width = 0)
    tree.column('#1', stretch = NO, minwidth = 0, width = 150)
    tree.column('#2', stretch = NO, minwidth = 0, width = 125)
    tree.column('#3', stretch = NO, minwidth = 0, width = 200)
    tree.pack()

    root.mainloop()

print("ID of process running main program: {}".format(os.getpid()))

print("Main thread name: {}".format(threading.current_thread().name))

t1 = threading.Thread(target=tampilWindow, name = 't1')
t2 = threading.Thread(target=task1, name = 't2')
t1.start()
t2.start()

t1.join()
t2.join()