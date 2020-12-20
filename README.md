# CS301 Course Project


## Requirement Analysis
In this course project, we are required to realize communication using wireless modules. Details of requirements are as follows:
![avatar](/photos/requirements.png)
Our group chose to use Bluetooth module to implement those functions. 
Some requirements should be implemented by using AT command, such as initiating and building connection between different Bluetooth modules. Therefore, we need to set our module into AT mode. 
## Solutions
The program mainly contains two parts: Bluetooth transmission model and LCD screen model. The Bluetooth model sends and receives  the message, then LCD model show them on the screen.

​	For LCD model, the main functions are as below:

      1. Show the messages from Bluetooth and local serial port separately.
      2. Able to slide the viewing window to see the history messages.
      3. Show the state of connection.
      4. A beautiful UI with proper font size, color and message box.
## Code Analysis
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
## Problems
