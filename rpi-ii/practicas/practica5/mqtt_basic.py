from time import sleep
from typing import Set, Dict
import random
from functools import reduce
from cbor2 import dumps
import paho.mqtt.client as mqtt


id_edificio : str = "EDIFICIO_PAALCALD"
plantas : Set[str] = set("P_" + str(x) for x in [0, 1, 2, 3, 4, 5, 6, 7, 8])
alas : Set[str] = set(["-N-", "-S-", "-E-", "-O-"])
salas : Set[int] = set([0, 1, 2, 3, 4, 5, 6, 7, 8])
sensores = {
    "TEMP": 
        {
            "name": "temperatura",
            "range":
            {
                "min" : 0,
                "max" : 50
            },
        },
    "HUM":
        {
            "name": "humedad",
            "range":
            {
                "min" : 0,
                "max" : 100
            }
        },
    "LUX":
        {
            "name": "luminosidad",
            "range": 
            {
                "min": 0,
                "max": 100,
            },

        },
    "VIBR": 
        {
            "name": "vibración",
            "range":
            {
                "min": 0,
                "max": 100
            },
        },
}

def get_random_place():
    """_summary_

    Returns:
        _type_: _description_
    """
    return reduce(lambda a, b: a + "/" + b,
                  list(map(lambda x: str(random.choice(list(x))), [plantas, alas, salas])),
                  "/" + id_edificio)

def get_measurements():
    return dict([sensor["name"], random.randint(sensor["range"]["min"], sensor["range"]["max"])]\
            for sensor in sensores.values())

def get_measurement():
    key= random.choice(list(sensores.keys()))
    place = get_random_place() + "/" + key
    me = {sensores[key]["name"]: random.randint(sensores[key]["range"]["min"], sensores[key]["range"]["max"])}
    return (place, me)

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, reason_code, properties):
    print(f"Connected with result code {reason_code}")
    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("$SYS/#")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))

mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.username_pw_set(username="friend", password="mellon");
mqttc.on_connect = on_connect
mqttc.on_message = on_message

mqttc.connect("127.0.0.1", 1883, 60)

# Blocking call that processes network traffic, dispatches callbacks and
# handles reconnecting.
# Other loop*() functions are available that give a threaded interface and a
# manual interface.
mqttc.loop_start()

while True:
    sleep(random.uniform(0, 1))
    measurement = get_measurement()
    mqttc.publish(measurement[0], dumps(measurement[1]))

mqttc.loop_stop()
