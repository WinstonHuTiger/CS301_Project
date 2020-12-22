# CS301 Course Project


## Requirement Analysis
In this course project, we are required to realize communication using wireless modules. Details of requirements are as follows:
![avatar](/photos/requirements.png)
Our group chose to use Bluetooth module to implement those functions. 
Some requirements should be implemented by using AT command, such as initiating and building connection between different Bluetooth modules. Therefore, we need to set our module into AT mode. 

## Solutions
The program mainly contains two parts: Bluetooth transmission model and LCD screen model. The Bluetooth model sends and receives  the message, then LCD model show them on the screen.

​	For Bluetooth communication model, the main functions are as below:


  1. Using USART to communicate between PC serial ports and Bluetooth serial ports, transpond normal messages.
  2. Offer interface to set AT commant in Bluetooth chip.
  3. Offer query interfaces for LCD to query some commonly used information.


​	For LCD model, the main functions are as below:

   1. Show the messages from Bluetooth and local serial port separately.
   2. Able to slide the viewing window to see the history messages.
   3. Show the state of connection.
   4. A beautiful UI with proper font size, color and message box.
   
The interfaces between two modules:

```c
// implemented by Bluetooth module:
char* get_state(); // get the current state string, for LCD to display.

// implemented by LCD module:
void add_message(unsigned char msg[], int is_self); // add message to current message queue. is_self denotes whether the message is sent or received.
```

## Code Analysis

### Bluetooth Communication

The Bluetooth Communication module offer these interfaces:

```c
void set_cmd(uint8_t *cmd); // execute AT command
char* get_state(); // get the current state string, for LCD to display.
void default_connect(); // start default connetcion, used when key0 is pressed.
void disconnect(); // disconnet current connection, used when key1 is pressed.
```

We implemented these interfaces by using interruption and global variables (as flags).

We set the serial port from PC as USART1, and serial port from bluetooth as USART2, the **pseudocode** for the UART callback function is:

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (from_USART1) { // when received from PC
		if (recieving_end) {
			if (message starts with ":") { // AT mode
                set_cmd(message);
                send_back_AT_response()
			} else { // normal message
				send_to_bluetooth(message); 
				add_message(message, is_self); // add to LCD display
			}
		} else { // not_recieved_end
			add_to_message_buffer();
		}
	} else { // when recieved from bluetooth
		if (end_with_normal_mode_ending) { // msg of normal mode
            send_to_pc(message); 
			add_message(message, not_self);
		} else if (end_with_AT_response_ending) { // answer of AT mode
            set global flag AT_flag to be 0;
			send_to_pc(message); 
		} else { // not_recieved_end
			add_to_message_buffer();
		}
	}
}
```

Here, we made the interrupt of USART2 (Bluetooth) the highest, thus to made the AT response could be handle timely.

The <code>set_cmd(uint8_t *cmd)</code> function have the following pseudocode:

```c
void set_cmd(uint8_t *cmd) {
	set global flag AT_flag = 1;
	HAL_GPIO_WritePin(BLEN_GPIO_Port, BLEN_Pin, GPIO_PIN_SET); // set output key as high, enter AT mode
	HAL_Delay(50);
	send_to_bluetooth(cmd);
	wait until AT_flag is zero or timeout;
	HAL_GPIO_WritePin(BLEN_GPIO_Port, BLEN_Pin, GPIO_PIN_RESET); // set output key as low, exit AT mode
}
```

Here, we need to wait for AT response message from Bluetooth module after we sent the AT command, the global AT_flag is used here. After the response handled in the Bluetooth callback, the AT_flag would be set to 0, thus the set_cmd could end.

The <code>get_state()</code> function is a packaging of <code>set_cmd(uint8_t *cmd)</code>, it returns a pure state string.

For default connection and disconnection, we have pseudocode:

```c
void default_connect() {
	if (not_currently_connected) {
		switch_role();
	} // else do nothing
}

void disconnect() {
	if (currently_connected) {
		switch_role();
		set_cmd("AT+DISC");
	} // else do nothing
}
```

Here, the <code>AT+DISC</code>  command can tentatively disconnect the connection, but it will soon reconnect by default it we didn't change the role. Also, if currently not connected, it we change the role to made the two Bluetooth modules pairable, it can build the connection by default.

### LCD Display
​	The LCD code mainly has three functions:

​    **add_message(unsigned char bluetooth_msg[], int is_self):** add message to **Data_lcd** with necessary information like rowlength and sides.

```c
void add_message(unsigned char bluetooth_msg[], int is_self) {
   int bias = 0;
   int len = strlen(bluetooth_msg);
   if (!checkhistory(bluetooth_msg)) { //如果传来的消息不是查询历史的消息就可以加入真实消息
      int tmp = base;
      while (len > 0) {//循环赋值
         if (len >= length_of_one_row) {
            for (int n = 0; n < length_of_one_row; n++)
               Data_lcd[base][n] = bluetooth_msg[n
                     + bias * length_of_one_row];
            rowLength[base] = length_of_one_row;
         } else {
            for (int n = 0; n < len; n++)
               Data_lcd[base][n] = bluetooth_msg[n
                     + bias * length_of_one_row];
            rowLength[base] = len;
         }
         side[base] = is_self;
         base++;
         bias++;
         len -= length_of_one_row;
      }

      DrawRectangle[tmp][0] = 1;
      DrawRectangle[tmp][1] = base - tmp;//这条消息会占几行，消息框要多大
      start = base - number_of_show_rows;//更新屏幕显示的基点
      if (start < 0)
         start = 0;
   }
   new_node_flag = 1;
}
```

​	**ScreenShow()**: show **Data_lcd** on the screen and draw the corresponding frame.

```c
void ScreenShow() {
   int winstart = start;
   int line = 0;
   POINT_COLOR = WHITE;
   LCD_Fill(field and color);
   for (int n = winstart; n < winstart + number_of_show_rows; n++) {
      if (Data_lcd[n][0] != '\0') {
         if (side[n] == 0) { //right
            //#########################content###############
            POINT_COLOR = GREEN;
            BACK_COLOR = BLACK;
            LCD_ShowString(paramter and data); 
            //###########################frame################
            if (DrawRectangle[n][0] == 1) {
               POINT_COLOR = YELLOW;
               LCD_DrawRectangle(field);
            }

         } else { //left
            POINT_COLOR = WHITE;
            BACK_COLOR = BLACK;
            LCD_ShowString(paramter and color);
            if (DrawRectangle[n][0] == 1) {
               POINT_COLOR = YELLOW;
               LCD_DrawRectangle(field);
            }
         }
      }
      line++;
   }
}
```

**checkhistory(unsigned char bluetooth_msg[]):** check消息是否是用来显示历史记录的指令

```c
int checkhistory(unsigned char bluetooth_msg[]) {
   if (strlen(bluetooth_msg) == 3) {
      sprintf(debugmsg, "check: ");
      HAL_UART_Transmit(&huart1, debugmsg, 20, 0xffff);
      int x = 0;
      int y = 0;
      while (bluetooth_msg[x] == up_msg[x])
         x++;
      while (bluetooth_msg[y] == down_msg[y])
         y++;
      if (x == 4) { 
         if (start > 0)
            start--; //控制显示向上
         sprintf(debugmsg, "up\n");
         HAL_UART_Transmit(&huart1, debugmsg, 20, 0xffff);
         return 1;
      }
      if (y == 4) {
         if (start < base)
            start++; //控制显示向下
         sprintf(debugmsg, "down\n");
         HAL_UART_Transmit(&huart1, debugmsg, 20, 0xffff);
         return 1;
      }
   }
   return 0;
}
```
## Problems & Solutions

**Problem:** The Bluetooth module cannot work when EN port is set high.

​	    **Solution:** Buy new HC-05 Bluetooth modules .

**Problem:** Cannot deal with AT response correctly.

​	    **Solution:**  Debugging by direct connect HC-05 and PC, found response could be multi-line, then solved.

**Problem:** AT command not executed.

​	    **Solution:**  Found that delay need to be introduced.

**Problem:** Cannot receive AT response. 

​	    **Solution:** Set the interrupt of Bluetooth USART as highest.

**Problem:** Not sure whether the output pin output the correct voltage.

​	    **Solution:**  Use multi-meter to measure voltage.

**Problem:** How to clear the old message in LCD screen?

​	    **Solution:**  LCD_Clear() to clear the whole screen, and using LCD_Fill() to clear locally is better.

## Result

### demo1: demonstrate the communication process

Using serial debug assistant to communicate as below:

<img src=".\photos\image-20201220123426779.png" height="380" width="600" align="center">

<img src=".\photos\image-20201220123538703.png" height="380" width="600" align="center">

The LCD screen:

<img src=".\photos\image-20201220122947403.png" height="350" width="600" align="center">

### demo2: demonstrate the page-turning function

The screen starts like below:

<img src=".\photos\image-20201220123954073.png" height="350" width="600" align="center">

After we input the <code>+++</code>in serial debug assistant:

<img src=".\photos\image-20201220124155902.png" height="350" width="600" align="center">

### demo3: demonstrate how to connecting and disconnecting the modules

At the beginning the modules are in the connected status:

<img src=".\photos\image-20201220124524751.png" height="380" width="600" align="center">

After we press the KEY1, the connection will shut down:

<img src=".\photos\image-20201220124719016.png" height="380" width="600" align="center">

Then we press the KEY0 to establish the connection again:

<img src=".\photos\image-20201220124803750.png" height="380" width="600" align="center">

## Thanks
Special thanks to 张柯远，吴凯越，香佳宏，陈思宇 for aids in Bluetooth module connection problem.
