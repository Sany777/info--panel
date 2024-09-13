
<h2 align="center">
IoT dashboard based on e-paper and esp32.
</h2>
The device is based on ESP32, 2.9 inch three-color e-paper from weact studio and a battery. Touch buttons are used for control, and the bmp280 sensor is used to measure the temperature in the room. Almost all the time, the device is in sleep mode, waking up on a timer - to update data or on a signal from the touch button. Touching one button starts the server to configure the device, while pressing the other button starts the data update.

