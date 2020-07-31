# EnigmaIOT Node example

This is the equivalent to EnigmaIOT node but this example does not put microcontroller in deep sleep state. It only sends a message with mocked values every 10 seconds.

As it is a non sleepy node it has clock synchronization available. This shows how to manage unixtime information. It dumps local time just before sending data.

In order to adapt it to your needs you only have to modify this code to readout sensor values.

```c++
void showTime () {
	const char* TZINFO = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00"; // Local TZ in Spain. Check https://remotemonitoringsystems.ca/time-zone-abbreviations.php
	
	tm timeinfo;
	static time_t displayTime;
	
	if (EnigmaIOTNode.hasClockSync()) {
		setenv ("TZ", TZINFO, 1);
		displayTime = millis ();
		time_t local_time_ms = EnigmaIOTNode.clock ();
		time_t local_time = EnigmaIOTNode.unixtime ();
		localtime_r (&local_time, &timeinfo);
		Serial.printf ("%02d/%02d/%04d %02d:%02d:%02d\n",
					   timeinfo.tm_mday,
					   timeinfo.tm_mon + 1,
					   timeinfo.tm_year + 1900,
					   timeinfo.tm_hour,
					   timeinfo.tm_min,
					   timeinfo.tm_sec);
	} else {
		Serial.printf ("Time not sync'ed\n");
	}
}

loop () {
    EnigmaIOTNode.handle (); // Needed to keep EnigmaIOT connection updated
    
	CayenneLPP msg (20);

	static time_t lastSensorData;
    static const time_t SENSOR_PERIOD = 10000;
    if (millis () - lastSensorData > SENSOR_PERIOD) {
        lastSensorData = millis ();
        showTime ();
        // Read sensor data
        msg.addAnalogInput (0, (float)(ESP.getVcc ()) / 1000);
        msg.addTemperature (1, 20.34);

        EnigmaIOTNode.sendData (msg.getBuffer (), msg.getSize ());
    }
}
```

This is the output in serial port

```
31/07/2020 13:31:14
Vcc: 2.994000
Trying to send: 00 02 01 2B 01 67 00 CB 
---- Data sent
31/07/2020 13:31:24
Vcc: 2.994000
Trying to send: 00 02 01 2B 01 67 00 CB 
---- Data sent
31/07/2020 13:31:34
Vcc: 2.994000
Trying to send: 00 02 01 2B 01 67 00 CB 
---- Data sent
```

