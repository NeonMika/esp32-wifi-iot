import paho.mqtt.client as mqtt # pip install paho-mqtt
import datetime
import time
import pprint
import pymongo # pip install pymongo pymongo[srv]
import os
import re

if "IOT_DB" not in os.environ:
    print("IOT_DB environment variable not set, has to contain the connection string for the database!\nExiting mqtt monitor now.")
    exit(-1)
if "MQTT_IP" not in os.environ:
    print("MQTT_IP environment variable not set, has to contain the IP adress of the MQTT server!\nExiting mqtt monitor now.")
    exit(-2)
client = pymongo.MongoClient(os.environ["IOT_DB"])
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
      x = messagesDb.temp.insert_one({"topic": topic, "mac": mac, "sensorNr" : sensorNr, "temp" : payload, "timestamp": timestamp})
      print(f'Inserted document with id {x.inserted_id}.')
        
    #print("==================")
    #print(client)
    #print(userdata)
    #print(msg)
    #print("==================")
    #print()

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

mqtt_ip = os.environ["MQTT_IP"]
client.connect(mqtt_ip, 1883, 60) 

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
