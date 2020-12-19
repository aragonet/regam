# regam

Watering home assistant.

Tired of hearing: "You are also one of those who kill plants"?
Find out the moisture of your lovely plants wherever you are and water them if needed!

Probably if you have been reading til here is because i'm you secret Santa :)

## Instructions

Place Regam box next to the plant you want to monitor, and remember that the more sun hours it has the more battery will have.
Once oplaced in the correct place, please, insert the soil moisture sensors carefully inside the plants soil.

Now we'll proceed to configure Regam in order to recive the state of the moisture.

Regam box can be opened by the top flag. Open it gently and you will find all the circuitry. Look for the jumper that is only connected to one pin and connect it to the other pin so that the device is switched on.

Now the device will have turned on, you'll need to use an app to send messages via bluetooth (BLE Scanner for example is recommended) in order to configure the wifi access so that Rega'm can warn you of the humidity of the plant at any time.
When connected to the device, you'll see two characteristics. The first one allows you to read all the wifi networks available, in case you forgot how your wifi is named. In the second one, is allows you to set the wifi ssid, the wifi pass and the url to post the message. The values need to be inserted on a json like this one: {"wifi": "xxxxxx", "pass": "xxxxxx", "post": "xxxxxx"}.
Once the message is sent, the values will be stored and will attempt to capture the soil humidity and will send a request to the url specified.
When the message is send, the percentage of the humidity will be concatenated to the end of the post url. (For testing purposes telegram api is recomended, but be free to use whatever endpoint you wish)

If you do not receive the message in less than a minute, probably the configuration has been wrong. Please, repeat the procedure until it works.

Once the message is sent, the device will be sleeped for the next two hours and then will send you again the state of the soil's moisture.


If the device behaves in an unexpectedly way, feel free to make any changes to the firm or the hardware, all PR are welcome!