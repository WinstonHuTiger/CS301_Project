# Bluetooth module ATK-HC05 Instruction

## 1.Hardware connection

### Connection between ATK-HC05 bluetooth module and board

| ATK-HC05 Bluetooth serial module |     VCC     |   GND   |   TXD   |   RXD   |   KEY   |   LED   |
| :------------------------------: | :---------: | :-----: | :-----: | :-----: | :-----: | :-----: |
|     **ALIENTEK STM32 Board**     | **3.3V/5V** | **GND** | **PA3** | **PA2** | **PC4** | **PA4** |

The USART serial port of the development board should be connected to the computer serial port.



## 2.Software operation

1. Turn on the board
2. Open the serial debug assistant in your computer
3. The power-on automatically establishes the default connection, and a **CONNECTED** appears at the LCD screen to indicate a successful connection
4. KEY0 establishes a default connection, and KEY1 closes the default connection
5. Enter a statement in the serial debug assistant and join the line break**(\n)** to communicate
6. send <code>+++</code> to turn the page up，send <code>---</code> to turn the page down
7. Set colon(**:**) as  the beginning indicating a AT instruction，for example <code>：AT+name?</code> to query the name of the current Bluetooth module；<code>：AT+name=xxx</code> to rename the name of the current Bluetooth module to **xxx**；<code>：AT+disc</code> to disconnect the modules and so on.

