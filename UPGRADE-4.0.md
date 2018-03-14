# Upgrade from 3.x to 4.0

There is one change that breaks compatibility with older versions. You'll have to change data type for all your binary resources such as images and fonts from

```c
const char MySymbol[] PROGMEM = {
```

to

```c
const uint8_t MySymbol[] PROGMEM = {
```
