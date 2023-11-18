Danfoss Eco
=============

The ``danfoss_eco`` climate platform creates a climate device which can be used to control a Danfoss Eco eTRV.

This component supports the following functionality:

- Switch between Manual and Schedule modes
- Set the target room temperature
- Show the current temperature
- Show current action (Heating/Idle)
- Show remaining battery level
- Managing multiple eTRVs from a single ESP32
- Reporting eTRV error status (with error codes)

This platform uses the ESP32 BLE peripheral on ESP32, which means, ``ble_client`` configuration should be provided.

Onboarding your device
------------------------
You should know the MAC address of your Danfoss Eco in order to complete the configuration. This information can be obtained in two ways:
1. Look it up in Danfoss Eco mobile app: `Settings->System Information->MAC Address`
2. Use `danfoss_eco_scanner` sensor and check the EspHome logs. Sample configuration for the scanner:
```yaml
esphome:
  name: etrv2wifi-scanner
  platform: ESP32
  board: esp32dev
  libraries:
    - xxtea-iot-crypt@2.0.1

logger:
  level: INFO

external_components:
  - source: github://dmitry-cherkas/esphome-danfoss-eco@v1.1.4

sensor:
  - platform: danfoss_eco_scanner
    id: scanner
```
When the scanner is running, press the hardware button on your Danfoss Eco in order to speed up the discovery. Sample scanner output will look like this:
```
[01:40:19][I][danfoss_eco_scanner:027]: Found Danfoss eTRV, MAC: 00:04:2F:xx:yy:zz, Name: 0;0:04:2F:xx:yy:zz;eTRV
```

Once the MAC Adress is known, esphome component can be configured as follows:
```yaml
external_components:
  - source: github://dmitry-cherkas/esphome-danfoss-eco@v1.1.4
ble_client:
  - mac_address: 00:04:2f:xx:yy:zz
    id: room_eco
climate:
  - platform: danfoss_eco
    name: "My Room eTRV"
    ble_client_id: room_eco
    battery_level:
      name: "My Room eTRV Battery Level"
    temperature:
      name: "My Room eTRV Temperature"
    update_interval: 30min
```

### Obtaining the `secret_key`
Danfoss Eco is using encrypted communication, which relies on the `secret_key`. This key can be obtained only if the hardware button was pressed before Bluetooth connection is established. Keep track of the EspHome logs to know, when the button should be pressed:
```
[01:25:01][I][logger:214]: Log initialized
[01:25:01][I][app:029]: Running through setup()...
[01:25:02][I][danfoss_eco:215]: [My Room eTRV] Short press Danfoss Eco hardware button NOW in order to allow reading the secret key
```
If the button was not pressed in time, failure to retrieve the `secret_key` will be logged. Restart the ESP32 and try again.
```
[01:25:13][W][danfoss_eco:148]: [My Room eTRV] Danfoss Eco hardware button was not pressed, unable to read the secret key
```

When component succeeds to read the `secret_key`, it will store the value in ESP32 flash, but it's recommended to explicitly add it to esphome component configuration.
```
[01:21:27][I][danfoss_eco:164]: [My Room eTRV] Consider adding below line to your danfoss_eco config:
[01:21:27][I][danfoss_eco:165]: [My Room eTRV] secret_key: deadbeefcafebabedeadbeefcafebabe 
[01:21:27][I][danfoss_eco:180]: [My Room eTRV] secret_key was saved to flash
```

Configuration options
------------------------

- **id** (*Optional*): Manually specify the ID used for code generation.
- **name** (**Required**, string): The name of the climate device.
- **ble_client_id** (**Required**): The ID of the BLE Client.
- **pin_code** (**Optional**, string): Device PIN code (if configured). Should be 4 characters numeric string.
- **secret_key** (**Required**, string): Device encryption key, 16 characters.
- **battery_level** (**Optional**, string): Remaining battery level sensor name. Sensor will not be created, if the name is not provided.
- **temperature** (**Optional**, string): Current temperature (Celsius) sensor name. Sensor will not be created, if the name is not provided.

> **NOTE:** Find more configuration examples in the repository root folder.


See Also
--------

This component is based on the work of other authors:
* [AdamStrojek libetrv](https://github.com/AdamStrojek/libetrv) (with additional features from [spin83](https://github.com/spin83/libetrv) fork)
* MQTT bridge for Danfoss eTRV by [keton](https://github.com/keton/etrv2mqtt) and Home Assistant add-on by [HBDK](https://github.com/HBDK/Eco2-Tools)

Other implementations:
* [Source code](https://github.com/dsltip/Danfoss-BLE) for controlling Danfoss ECO via cc2541 as a gateway from the desktop PC.
