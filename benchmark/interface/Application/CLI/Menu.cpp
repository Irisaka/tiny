#include "Menu.hpp"
#include "usart.h"
#include "tim.h" // jhdbg only needed for debug

#define RESPONSE_QUEUE_SIZE     20 * sizeof(ULONG)

namespace CLI
{
  Menu::Menu(TX_BYTE_POOL &byte_pool, IO::Uart &uart, const menu_command_t *commands): uart(uart), commands(commands)
  {
    static const char *name = "Response Queue";
    VOID *pointer;
    tx_byte_allocate(&byte_pool, &pointer, RESPONSE_QUEUE_SIZE, TX_NO_WAIT);
    tx_queue_create(&queue, (char *)name, TX_1_ULONG, pointer, RESPONSE_QUEUE_SIZE);
  }

  void Menu::Run()
  {
    while(1)
    {
      const std::string *buffer = uart.ReadUntil("%");
      HandleCommand(*buffer);
      delete buffer;

	  // HAL_GPIO_WritePin(WW_DET_IN_GPIO_Port, WW_DET_IN_Pin, GPIO_PIN_SET);
      // int delay_start = __HAL_TIM_GET_COUNTER(&htim2);
      // while(__HAL_TIM_GET_COUNTER(&htim2) < delay_start + 100 ){
      // 	;
      // }
      // HAL_GPIO_WritePin(WW_DET_IN_GPIO_Port, WW_DET_IN_Pin, GPIO_PIN_RESET);

      // int pin_val = HAL_GPIO_ReadPin(WW_DET_IN_GPIO_Port, WW_DET_IN_Pin);
      // std::string message = "pin is " + std::to_string(pin_val);
      // uart.SendString(message);
    }
  }

  void Menu::HandleCommand(const std::string &buffer)
  {
    UINT buf_len = buffer.length();
    for(int i = 0; buf_len > 0 && commands[i].command.length() != 0; i++)
    {
      if(buffer.rfind(commands[i].command.c_str(), 0) == 0) // buffer matches the i'th command
      {
        if(buf_len == commands[i].command.length())
        {
          // command in the buffer has no arguments
          commands[i].action("");
        }
        else if(buf_len > commands[i].command.length() + 1
                && buffer[commands[i].command.length()] == ' ')
        {
          // command in the buffer has additional text beyond just the command
          commands[i].action(&buffer[commands[i].command.length() + 1]);
        }
        return;
      }
    }
    Default(buffer);
  }

  void Menu::SendString(const std::string &string)
  {
    uart.SendString(string.c_str());
  }

  void Menu::SendString(const char *string)
  {
    uart.SendString(string);
  }

  void Menu::SendEndLine()
  {
    SendString("\r\n");
  }

  void Menu::SendEnd()
  {
    SendString("m-ready");
    SendEndLine();
  }

  void Menu::Default(const std::string &args)
  {
    SendEnd();
  }

  void Menu::SendResponse(const std::string *prefix)
  {
    std::string *line;

    UINT result = tx_queue_receive(&queue, &line, 500 * TX_TIMER_TICKS_PER_SECOND / 1000);
    while(result == TX_QUEUE_EMPTY || (result == TX_SUCCESS && line != TX_NULL))
    {
      if(result == TX_SUCCESS)
      {
        if(prefix != TX_NULL)
          SendString(*prefix);
        SendString(*line);
        SendEndLine();
        delete line;
      }
      result = tx_queue_receive(&queue, &line, 500 * TX_TIMER_TICKS_PER_SECOND / 1000);
    }
  }
}
