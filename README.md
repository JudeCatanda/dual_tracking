# Wiring Table

## Power rails (breadboard)
| From | To |
|---|---|
| Arduino **5V** | Breadboard **+ rail** |
| Arduino **GND** | Breadboard **- rail** |
| External 5V supply **(-)** | Breadboard **- rail** (and any Arduino GND) |
| External 5V supply **(+)** | Servo red wires only (not the Arduino 5V) |

## DS3231 RTC (I2C)
| RTC pin | Connects to |
|---|---|
| VCC | + rail (5V) |
| GND | - rail (GND) |
| SDA | Arduino **A4** |
| SCL | Arduino **A5** |
| SQW, 32K | Not connected |

## SD card module (SPI)
| SD module pin | Connects to |
|---|---|
| VCC | + rail (5V) |
| GND | - rail (GND) |
| CS | Arduino **D10** |
| MOSI (DI / SDI) | Arduino **D11** |
| MISO (MOSO / DO / SDO) | Arduino **D12** |
| SCK | Arduino **D13** |

## LDRs (four voltage dividers)
| LDR position | LDR leg 1 | LDR leg 2 and 10k resistor |
|---|---|---|
| Top-Left | + rail (5V) | Arduino **A0**, plus 10k from A0 to - rail |
| Bottom-Left | + rail (5V) | Arduino **A1**, plus 10k from A1 to - rail |
| Bottom-Right | + rail (5V) | Arduino **A2**, plus 10k from A2 to - rail |
| Top-Right | + rail (5V) | Arduino **A3**, plus 10k from A3 to - rail |

## Servos
| Servo | Signal (orange/yellow) | Power (red) | Ground (brown/black) |
|---|---|---|---|
| Horizontal | Arduino **D5** | External 5V (+) | External supply (-) / common GND |
| Vertical | Arduino **D4** | External 5V (+) | External supply (-) / common GND |

## Extra
| Part | Connection |
|---|---|
| 470-1000µF capacitor | Across the external 5V supply (+ leg to +, stripe/- leg to -), close to the servos |

## Arduino Uno pins
| Arduino pin | Connected to |
|---|---|
| 5V | + rail |
| GND | - rail |
| A0 | LDR Top-Left |
| A1 | LDR Bottom-Left |
| A2 | LDR Bottom-Right |
| A3 | LDR Top-Right |
| A4 | RTC SDA |
| A5 | RTC SCL |
| D4 | Vertical servo signal |
| D5 | Horizontal servo signal |
| D10 | SD CS |
| D11 | SD MOSI |
| D12 | SD MISO (MOSO) |
| D13 | SD SCK |
| D0-D3, D6-D9 | Free |
