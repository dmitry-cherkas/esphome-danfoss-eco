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

This platform uses the ESP32 BLE peripheral on ESP32, which means, ``ble_client`` configuration should be provided.

> **NOTE:** At this point, component does not offer the means to discover eTRV MAC address and secret key.
> This can be done with other tools, check [here](https://github.com/keton/etrv2mqtt) and [here](https://github.com/HBDK/Eco2-Tools).

Simple configuration will look similar to this:
```yaml
external_components:
  - source: github://dmitry-cherkas/esphome-danfoss-eco@v1.0.
ble_client:
  - mac_address: 00:04:2f:xx:xx:xx
    id: room_eco
climate:
  - platform: danfoss_eco
    name: "My Room eTRV"
    ble_client_id: room_eco2
    pin_code: "0000"
    secret_key: deadbeefcafebabedeadbeefcafebabe 
    battery_level:
      name: "My Room eTRV Battery Level"
    temperature:
      name: "My Room eTRV Temperature"
    update_interval: 30min
```

Configuration variables:
------------------------

- **id** (*Optional*): Manually specify the ID used for code generation.
- **name** (**Required**, string): The name of the climate device.
- **ble_client_id** (**Required**): The ID of the BLE Client.
- **pin_code** (**Optional**, string): Device PIN code (if configured). Should be 4 characters numeric string.
- **secret_key** (**Required**, string): Device encryption key, 16 characters.
- **battery_level** (**Optional**, string): Remaining battery level sensor name. Sensor will not be created, if the name is not provided.
- **temperature** (**Optional**, string): Current temperature (Celsius) sensor name. Sensor will not be created, if the name is not provided.

See Also
--------

This component is based on the work of other authors:
* [AdamStrojek libetrv](https://github.com/AdamStrojek/libetrv) (with additional features from [spin83](https://github.com/spin83/libetrv) fork)
* MQTT bridge for Danfoss eTRV by [keton](https://github.com/keton/etrv2mqtt) and Home Assistant add-on by [HBDK](https://github.com/HBDK/Eco2-Tools)

Other implementations:
* [Source code](https://github.com/dsltip/Danfoss-BLE) for controlling Danfoss ECO via cc2541 as a gateway from the desktop PC.