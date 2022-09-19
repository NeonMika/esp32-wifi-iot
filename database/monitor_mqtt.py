import paho.mqtt.client as mqtt # pip install paho-mqtt
import datetime
import time
import pprint
import pymongo # pip install pymongo pymongo[srv]
import os
import re

client = pymongo.MongoClient("mongodb+srv://USER:PASSWORD@DATABASE?retryWrites=true&w=majority") # TODO!
messagesDb = client.messages

pp = pprint.PrettyPrinter(indent=4)
latestMessages = {}

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("##################")
    print("### ON_CONNECT ###")
    print("##################")
    print(datetime.datetime.now())
    print("==================")
    print("Connected with result code " + str(rc))
    print("==================")
    print(client)
    print(userdata)
    print(flags)
    print("==================")

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("$SYS/#")
    print("Subscribed to $SYS/#")
    client.subscribe("#")
    print("Subscribed to #")
    print("==================")
    print()

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    #print("##################")
    #print("### ON_MESSAGE ###")
    #print("##################")    
    #print(datetime.datetime.now())
    #print("==================")
    #print(msg.topic)
    #print(msg.payload)
    topic = msg.topic
    payload = msg.payload.decode('UTF-8')
    timestamp = datetime.datetime.now()
    
    latestMessages[topic.ljust(40)] = payload
    
    p = re.compile('esp32\\/sensor-client\\/(.*)\\/temp\\/sensor\\/(\\d+)')
    m = p.match(topic)
    if(m is not None):
      mac = m.group(1)
      sensorNr = m.group(2)
      messagesDb.temp.insert_one({"topic": topic, "mac": mac, "sensorNr" : sensorNr, "temp" : payload, "timestamp": timestamp})
        
    #print("==================")
    #print(client)
    #print(userdata)
    #print(msg)
    #print("==================")
    #print()

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect("192.168.XXX.YYY", 1883, 60) # TODO!

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_start()

while True:
    os.system('cls')
    print(datetime.datetime.now())
    pp.pprint(latestMessages)
    time.sleep(2.5)
