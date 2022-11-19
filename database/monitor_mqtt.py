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

def subscribe_to_sys(client):
    client.subscribe("$SYS/#")
    print("Subscribed to $SYS/#")

def subscribe_to_hash(client):
    client.subscribe("#")
    print("Subscribed to #")


# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    print("##################")
    print("### ON_CONNECT ###")
    print("##################")
    print(datetime.datetime.now())
    print("==================")
    print("Connected with result code", rc)
    print("==================")
    print("client =", client)
    print("userdata =", userdata)
    print("flags", flags)
    print("==================")

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    # subscribe_to_sys(client)
    subscribe_to_hash(client)
    print("==================")
    print()

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    try:
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
    
        # Handle new temparature
        p = re.compile('esp32\\/sensor-client\\/(.*)\\/temp\\/sensor\\/(\\d+)')
        m = p.match(topic)
        if(m is not None):
            mac = m.group(1)
            sensorNr = m.group(2)
            roomNameCursor = messagesDb.names.find({"mac": mac}).sort("since", -1).limit(1)
            print("TEMP", timestamp, mac, sensorNr, payload, sep=' | ')
            try:
                roomDoc = roomNameCursor.next()
            except StopIteration:
                roomDoc = None
            roomName = roomDoc['name'] if roomDoc is not None else 'Unbekannter Raum'
            print('       ', roomName, sep='')
            x = messagesDb.temp.insert_one({"topic": topic, "room": roomName, "mac": mac, "sensorNr" : sensorNr, "temp" : payload, "timestamp": timestamp})
            print(f'       Inserted document with id {x.inserted_id}.')
        
        #print("==================")
        #print(client)
        #print(userdata)
        #print(msg)
        #print("==================")
        #print()
    except Exception as ex:
        with open("errors.txt", "a") as errors:
            errors.write(str(timestamp))
            errors.write(str(ex)) 
        print(timestamp)
        print(ex)

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

mqtt_ip = os.environ["MQTT_IP"]
client.connect(mqtt_ip, 1883, 60) 

# The loop_start() method starts a new thread, that calls the loop method at regular intervals for you. It also handles re-connects automatically. 
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
client.loop_start()

while True:
    try:
#        os.system('cls')
#        print(datetime.datetime.now())
#        pp.pprint(latestMessages)
        time.sleep(2.5)
    except Exception as ex:
        timestamp = datetime.datetime.now()
        with open("errors.txt", "a") as errors:
            errors.write(str(timestamp)) 
            errors.write(str(ex)) 
        print(timestamp)
        print(ex)
