# EnigmaIOT LED Flasher example

This example implements a node that illustrate time synchronization on nodes that do not sleep.

This feature is enabled by adding this line during Arduino `setup ()`

```c++
EnigmaIOTNode.enableClockSync ();
```

As EnigmaIOT Node example it sends mocked data.

Time synchronization is demonstrated by making board led flash synchronously on all nodes connected to same EnigmaIOT gateway using this code.

User code may make use of  `EnigmaIOTNode.clock ()` to get common clock. It is used in the same way as normal `micros()` call.

There are additional methods used to interrogate time synchronization status:

- `EnigmaIOTNode.hasClockSync()`  let you know if node has got a valid sync from gateway.
- `EnigmaIOTNode.unixtime()` gives the clock un real world time (if Gateway has got NTP sync)

