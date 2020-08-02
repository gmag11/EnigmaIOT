# EnigmaIOT SmartSwitch Controller

This example shows how all features of [EnigmaIOTjsonController.h](https://github.com/gmag11/EnigmaIOT/blob/dev/src/EnigmaIOTjsonController.h) may be used to build a complete node.

It controls a relay and gets messages by button presses. Button may be linked so that relay is toggled on every button press.

Three configurable parameters are added:

- Relay pin
- Button pin
- Initial relay status after boot: it may be either ON, OFF or SAVED. If this is "SAVED" node stores last status and keeps after a reboot.

Relay is controller with this command

```
<Network_name>/<node_name or node_address>/set/data  {"cmd":"rly","rly":< 1 or 0 >}
```

If button is pressed, this MQTT message is produced

```
<Network_name>/<node_name or node_address>/data  {"button":< button_pin >,"push":1}
```

To change button and relay link status you may use this message

```
<Network_name>/<node_name or node_address>/set/data  {"cmd":"link","link":< 1 or 0 >}
```

`1` makes button and relay to be linked, `0` makes that button does not produce any immediate effect in relay.

Initial relay state after boot may be configured with a MQTT message too.

```
<Network_name>/<node_name or node_address>/set/data  {"cmd":"bstate","bstate":< 0, 1 or 2 >}
```

`0` and `1` mean that LED will start in OFF or ON state respectively, and `2` means that status will be saved.

You may ask about these configurations using

```
<Network_name>/<node_name or node_address>/det/data  {"cmd":<command>}
```

Command field may be `rly`, `link` or `bstate`.

 