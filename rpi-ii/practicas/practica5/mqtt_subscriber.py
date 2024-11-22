import paho.mqtt.client as mqtt
from cbor2 import loads
from typing import Dict, Any
sensores : Dict[str, Dict[str, Any]] = {
    "TEMP": 
        {
            "name": "temperatura",
            "threshold":
            {
                "min" : 10,
                "max" : 40
            },
            "range":
            {
                "min" : 0,
                "max" : 50
            },
        },
    "HUM":
        {
            "name": "humedad",
            "threshold":
            {
                "min" : 10,
                "max" : 60
            },
            "range":
            {
                "min" : 0,
                "max" : 100
            }
        },
    "LUX":
        {
            "name": "luminosidad",
            "threshold":
            {
                "min" : 15,
                "max" : 100
            },
            "range": 
            {
                "min": 0,
                "max": 100,
            },

        },
    "VIBR": 
        {
            "name": "vibración",
            "threshold":
            {
                "min" : 10,
                "max" : 40
            },
            "range":
            {
                "min": 0,
                "max": 100
            },
        },
}

def on_subscribe(client, userdata, mid, reason_code_list, properties):
    # Since we subscribed only for a single channel, reason_code_list contains
    # a single entry
    if reason_code_list[0].is_failure:
        print(f"Broker rejected you subscription: {reason_code_list[0]}")
    else:
        print(f"Broker granted the following QoS: {reason_code_list[0].value}")

def on_unsubscribe(client, userdata, mid, reason_code_list, properties):
    # Be careful, the reason_code_list is only present in MQTTv5.
    # In MQTTv3 it will always be empty
    if len(reason_code_list) == 0 or not reason_code_list[0].is_failure:
        print("unsubscribe succeeded (if SUBACK is received in MQTTv3 it success)")
    else:
        print(f"Broker replied with failure: {reason_code_list[0]}")
    client.disconnect()

def on_message(client, userdata, message):
    # userdata is the structure we choose to provide, here it's a list()
    measurement_type = message.topic.split("/")[-1]
    measurement = int(loads(message.payload)[sensores[measurement_type]["name"]])
    # if (sensores[measurement_type]["threshold"]["min"] > measurement or measurement > sensores[measurement_type]["threshold"]["max"]):
    print(message.topic)
    print(loads(message.payload))

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code.is_failure:
        print(f"Failed to connect: {reason_code}. loop_forever() will retry connection")
    else:
        # we should always subscribe from on_connect callback to be sure
        # our subscribed is persisted across reconnections.
        # client.subscribe("/EDIFICIO_PAALCALD/+/+/+/TEMP")
        # client.subscribe("/EDIFICIO_PAALCALD/+/-O-/+/VIBR")
        # client.subscribe("/EDIFICIO_PAALCALD/P_7/-S-/4/#")
        client.subscribe("/EDIFICIO_PAALCALD/+/+/+/TEMP")
        client.subscribe("/EDIFICIO_PAALCALD/+/+/+/HUM")

mqttc = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
mqttc.username_pw_set(username="friend", password="mellon");
mqttc.on_connect = on_connect
mqttc.on_message = on_message
mqttc.on_subscribe = on_subscribe
mqttc.on_unsubscribe = on_unsubscribe

mqttc.user_data_set([])
mqttc.connect("127.0.0.1", 1883, 60)
mqttc.loop_forever()
