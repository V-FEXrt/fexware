/* Kernel includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

/* System Libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Board Libraries */
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"

/* Tiny USB */
#include "tusb.h"
#include "usb_descriptors.h"

/* Application Libraries */
#include "bsp/board.h"

/* Application Code */
#include "actions.h"
#include "filesystem.h"
#include "layer.h"
#include "parser.h"
#include "queue_message.h"
#include "tokenizer.h"

/* Task Stack Sizes */
#define USB_DEVICE_STACK_SIZE (3 * configMINIMAL_STACK_SIZE / 2) * (CFG_TUSB_DEBUG ? 2 : 1)
#define USB_HID_STACK_SIZE (configMINIMAL_STACK_SIZE)
#define POLL_KEYS_STACK_SIZE (512)
#define DRAW_DISPLAYS_STACK_SIZE (512)
#define BLINK_STACK_SIZE (configMINIMAL_STACK_SIZE)

/* Priorities at which the tasks are created. */
#define USB_DEVICE_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define USB_HID_TASK_PRIORITY (configMAX_PRIORITIES - 2)
#define POLL_KEYS_TASK_PRIORITY (configMAX_PRIORITIES - 3)
#define DRAW_DISPLAYS_TASK_PRIORITY (configMAX_PRIORITIES - 4)
#define BLINK_TASK_PRIORITY (tskIDLE_PRIORITY)

/* Task Periods */

// USB Device task doesn't have a period, it loops
// as fast as it can and blocks on an empty queue
// #define USB_DEVICE_TASK_PERIOD
#define USB_HID_TASK_PERIOD (10 / portTICK_PERIOD_MS)
#define POLL_KEYS_TASK_PERIOD (10 / portTICK_PERIOD_MS)
#define DRAW_DISPLAYS_TASK_PERIOD (500 / portTICK_PERIOD_MS)
#define BLINK_TASK_PERIOD (1000 / portTICK_PERIOD_MS)

/* Application Constants */
#define MESSAGE_QUEUE_LENGTH (1)
#define BLINK_TASK_LED (PICO_DEFAULT_LED_PIN)
#define CORE_0_AFFINITY_MASK (1 << 0)
#define CORE_1_AFFINITY_MASK (1 << 1)
#define ALL_CORES_AFFINITY_MASK (CORE_0_AFFINITY_MASK | CORE_1_AFFINITY_MASK)
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)     \
  (byte & 0x80 ? '1' : '0'),     \
      (byte & 0x40 ? '1' : '0'), \
      (byte & 0x20 ? '1' : '0'), \
      (byte & 0x10 ? '1' : '0'), \
      (byte & 0x08 ? '1' : '0'), \
      (byte & 0x04 ? '1' : '0'), \
      (byte & 0x02 ? '1' : '0'), \
      (byte & 0x01 ? '1' : '0')

static const unsigned char REG_IP0 = 0x00 | 0b10000000;

/* Static Task Handles */
StackType_t usb_device_task_stack[USB_DEVICE_STACK_SIZE];
StaticTask_t usb_device_task;

StackType_t usb_hid_task_stack[USB_HID_STACK_SIZE];
StaticTask_t usb_hid_task;

/* Dynamic Task Handles */

/*-----------------------------------------------------------*/

/*
 * Hardware Initialization
 */
static void prvHardwareInit(void);

/*
 * FreeRTOS Application Tasks
 */
static void prvUsbDeviceTask(void *pvParameters);
static void prvUsbHidTask(void *pvParameters);
static void prvPollKeysTask(void *pvParameters);
static void prvDrawDisplaysTask(void *pvParameters);
static void prvBlinkTask(void *pvParameters);

/*-----------------------------------------------------------*/

// Mutex not needed since only one task uses it
// Shared because main also uses it (before task scheduling begins)
fex::Filesystem fs;

// Mutex not needed since only one task uses it
// Shared because main initializes it before scheduling
int layer = 0;
std::unordered_map<int, std::pair<std::string, fex::Layer>> layers;

// OLED and Expander task both use I2C, should be mutexed
SemaphoreHandle_t xI2CMutex;

QueueHandle_t xMessageQueue;

// Should probably be a mutex, but I think a bool works for now
bool hid_send_complete = true;

// Parsing status
std::string parse_status = "Parse: Success";

/*-----------------------------------------------------------*/

int main(void)
{
  prvHardwareInit();

  printf("Starting fexware.\n");

  // printf("Erasing...\n");
  // fs.EraseAll();

  std::vector<std::string> files;
  bool keymap_boot_override = false;

  printf("Initializing filesystem\n");
  if (fs.Initialize())
  {
    printf("Mounting filesystem\n");
    fs.Mount();
    printf("Listing filesystem\n");
    files = fs.List("/");
    printf("Done listing filesystem\n");
  }

  printf("Collecting files on disk\n");
  std::vector<std::string> keymap_files;
  for (auto file : files)
  {
    printf("--- '%s'\n", file.c_str());

    int idx = file.rfind('.');
    if (idx != std::string::npos)
    {
      std::string ext = file.substr(idx + 1);
      if (ext == "kmf")
      {
        keymap_files.push_back(file.substr(2, idx - 2));
      }
    }
  }

  printf("Keymap Files:\n");
  for (auto file : keymap_files)
  {
    fex::Layer l;
    printf("===== %s =====\n", file.c_str());
    std::string source = fs.ReadFile("//" + file + ".kmf");
    std::string error = fex::parse_source(source, &l);
    printf("%s\n", source.c_str());
    printf("==============\n");
    printf("Parsing: '%s'\n", error.c_str());
    printf("==============\n");

    if (error != "")
    {
      parse_status = error;
    }

    if (file == "BaseLayer")
    {
      layer = std::hash<std::string>()("BaseLayer");
    }

    layers[std::hash<std::string>()(file)] = std::pair{file, std::move(l)};
  }

  // TODO(fex): might be an issue if Initalize fails
  fs.Unmount();

  xI2CMutex = xSemaphoreCreateMutex();
  if (xI2CMutex == NULL)
  {
    printf("---- FAILED TO CREATE MUTEX ----\n");
    return 1;
  }

  xMessageQueue = xQueueCreate(100, sizeof(fex::QueueMessage));
  if (xMessageQueue == NULL)
  {
    printf("---- FAILED TO CREATE QUEUE ----\n");
    return 1;
  }

  // TODO(fex): pressing a key twice will sometimes miss a press
  TaskHandle_t poll_keys_handle;
  TaskHandle_t draw_displays_handle;
  TaskHandle_t blink_handle;
  xTaskCreate(prvPollKeysTask, "poll_keys", POLL_KEYS_STACK_SIZE, NULL, POLL_KEYS_TASK_PRIORITY, &poll_keys_handle);
  xTaskCreate(prvDrawDisplaysTask, "draw_displays", DRAW_DISPLAYS_STACK_SIZE, NULL, DRAW_DISPLAYS_TASK_PRIORITY, &draw_displays_handle);
  xTaskCreate(prvBlinkTask, "blink", BLINK_STACK_SIZE, NULL, BLINK_TASK_PRIORITY, &blink_handle);

  TaskHandle_t usb_d_handle = xTaskCreateStatic(prvUsbDeviceTask, "usb_device", USB_DEVICE_STACK_SIZE, NULL, USB_DEVICE_TASK_PRIORITY, usb_device_task_stack, &usb_device_task);
  TaskHandle_t usb_hid_handle = xTaskCreateStatic(prvUsbHidTask, "usb_hid", USB_HID_STACK_SIZE, NULL, USB_HID_TASK_PRIORITY, usb_hid_task_stack, &usb_hid_task);

  // TinyUSB is super race-y and crashy when not running one a single core
  // Further, for the same reasons, it must be run on core 0
  vTaskCoreAffinitySet(usb_d_handle, CORE_0_AFFINITY_MASK);
  vTaskCoreAffinitySet(usb_hid_handle, CORE_0_AFFINITY_MASK);
  vTaskCoreAffinitySet(poll_keys_handle, CORE_0_AFFINITY_MASK);
  vTaskCoreAffinitySet(draw_displays_handle, CORE_1_AFFINITY_MASK);
  vTaskCoreAffinitySet(blink_handle, CORE_1_AFFINITY_MASK);

  vTaskStartScheduler();

  for (;;)
    ;

  return 0;
}

/*-----------------------------------------------------------*/

static void prvUsbDeviceTask(void *pvParameters)
{
  printf("Starting USB Device Task...\n");
  tusb_init();

  while (true)
  {
    // printf("core %d:  Usb Device\n", get_core_num());
    tud_task();
  }
}

/*-----------------------------------------------------------*/

static void prvPollKeysTask(void *pvParameters)
{
  printf("Starting Poll Keys Task...\n");
  TickType_t nextWake = xTaskGetTickCount();

  const uint8_t LEFT_ADDRESS = 0x23;
  const uint8_t RIGHT_ADDRESS = 0x27;

  // init i2c
  i2c_init(i2c1, 400 * 1000);
  gpio_set_function(6, GPIO_FUNC_I2C);
  gpio_set_function(7, GPIO_FUNC_I2C);
  gpio_pull_up(6);
  gpio_pull_up(7);

  xSemaphoreTake(xI2CMutex, portMAX_DELAY);

  printf("\nI2C Bus Scan\n");
  printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

  for (int addr = 0; addr < (1 << 7); ++addr)
  {
    if (addr % 16 == 0)
    {
      printf("%02x ", addr);
    }

    int ret;
    uint8_t rxdata;
    ret = i2c_read_blocking(i2c1, addr, &rxdata, 1, false);

    printf(ret < 0 ? "." : "@");
    printf(addr % 16 == 15 ? "\n" : "  ");
  }

  xSemaphoreGive(xI2CMutex);

  const int width = 12;
  int keys[] = {
      /* 0, 0 */ -1,
      /* 0, 1 */ -1,
      /* 0, 2 */ -1,
      /* 0, 3 */ -1,
      /* 0, 4 */ -1,
      /* 0, 5 */ 0 * width + 1,
      /* 0, 6 */ 0 * width + 2,
      /* 0, 7 */ 0 * width + 3,
      /* 1, 0 */ 2 * width + 4,
      /* 1, 1 */ 1 * width + 4,
      /* 1, 2 */ 0 * width + 4,
      /* 1, 3 */ 3 * width + 5,
      /* 1, 4 */ 2 * width + 5,
      /* 1, 5 */ 1 * width + 5,
      /* 1, 6 */ 0 * width + 5,
      /* 1, 7 */ 4 * width + 4,
      /* 2, 0 */ 3 * width + 6,
      /* 2, 1 */ 2 * width + 6,
      /* 2, 2 */ 1 * width + 6,
      /* 2, 3 */ 0 * width + 6,
      /* 2, 4 */ -1,
      /* 2, 5 */ 3 * width + 4,
      /* 2, 6 */ 4 * width + 3,
      /* 2, 7 */ 4 * width + 2,
      /* 3, 0 */ 1 * width + 3,
      /* 3, 1 */ 2 * width + 3,
      /* 3, 2 */ 3 * width + 3,
      /* 3, 3 */ -1,
      /* 3, 4 */ 1 * width + 2, // Broken key?
      /* 3, 5 */ 2 * width + 2, // Broken key?
      /* 3, 6 */ 3 * width + 2,
      /* 3, 7 */ 4 * width + 1,
      /* 4, 0 */ 1 * width + 1,
      /* 4, 1 */ 2 * width + 1,
      /* 4, 2 */ 0 * width + 0,
      /* 4, 3 */ 3 * width + 1,
      /* 4, 4 */ 1 * width + 0,
      /* 4, 5 */ 2 * width + 0,
      /* 4, 6 */ 3 * width + 0,
      /* 4, 7 */ 4 * width + 0, // Broken key?
      /* 5, 0 */ 1 * width + 11,
      /* 5, 1 */ 1 * width + 10,
      /* 5, 2 */ 1 * width + 9,
      /* 5, 3 */ 1 * width + 8,
      /* 5, 4 */ 1 * width + 7,
      /* 5, 5 */ 2 * width + 11,
      /* 5, 6 */ 2 * width + 10,
      /* 5, 7 */ 2 * width + 9,
      /* 6, 0 */ 2 * width + 8,
      /* 6, 1 */ 2 * width + 7,
      /* 6, 2 */ 3 * width + 11,
      /* 6, 3 */ 4 * width + 8,
      /* 6, 4 */ -1,
      /* 6, 5 */ 4 * width + 7,
      /* 6, 6 */ 4 * width + 6,
      /* 6, 7 */ 4 * width + 5,
      /* 7, 0 */ 3 * width + 7,
      /* 7, 1 */ 3 * width + 8,
      /* 7, 2 */ 3 * width + 9,
      /* 7, 3 */ 3 * width + 10,
      /* 7, 4 */ -1,
      /* 7, 5 */ -1,
      /* 7, 6 */ -1,
      /* 7, 7 */ -1,
      /* 8, 0 */ -1,
      /* 8, 1 */ 0 * width + 7,
      /* 8, 2 */ 0 * width + 8,
      /* 8, 3 */ 0 * width + 9,
      /* 8, 4 */ 0 * width + 10,
      /* 8, 5 */ 0 * width + 11,
      /* 8, 6 */ -2, // Button 0,0
      /* 8, 7 */ -2, // Button 1,0
      /* 9, 0 */ -2, // Button 2,0
      /* 9, 1 */ -1,
      /* 9, 2 */ -1,
      /* 9, 3 */ -1,
      /* 9, 4 */ -1,
      /* 9, 5 */ -2, // Button 0,1
      /* 9, 6 */ -2, // Button 1,1
      /* 9, 7 */ -2, // Button 2,1
  };

  int64_t timeouts[80];
  memset(timeouts, -1, sizeof(int64_t) * 80);
  TickType_t hold = pdMS_TO_TICKS(200);

  uint8_t msg[1] = {REG_IP0};
  uint8_t previous[10];
  uint8_t current[10];
  memset(previous, 0xFF, 10);
  memset(current, 0xFF, 10);
  while (true)
  {
    xTaskDelayUntil(&nextWake, POLL_KEYS_TASK_PERIOD);

    xSemaphoreTake(xI2CMutex, portMAX_DELAY);

    i2c_write_blocking(i2c1, LEFT_ADDRESS, msg, 1, false);
    int num_bytes_read = i2c_read_blocking(i2c1, LEFT_ADDRESS, current, 5, false);
    i2c_write_blocking(i2c1, RIGHT_ADDRESS, msg, 1, false);
    num_bytes_read = i2c_read_blocking(i2c1, RIGHT_ADDRESS, current + 5, 5, false);

    xSemaphoreGive(xI2CMutex);

    TickType_t now = xTaskGetTickCount();

    for (int i = 0; i < 10; i++)
    {
      for (int j = 0; j < 8; j++)
      {
        if (layers[layer].second.Bound(keys[i * 8 + j], fex::Operation::HOLD) && timeouts[i * 8 + j] != -1 && now - timeouts[i * 8 + j] > hold)
        {
          printf("holdng key: %d\n", timeouts[i * 8 + j]);
          layers[layer].second.Enqueue(keys[i * 8 + j], fex::Operation::HOLD, fex::BoundActionEnqueue::DO, xMessageQueue);
          timeouts[i * 8 + j] = -1;
        }
      }
    }

    for (int i = 0; i < 10; i++)
    {
      uint8_t prev = previous[i];
      uint8_t curr = current[i];
      for (int j = 0; j < 8; j++)
      {
        if ((prev & 1) != (curr & 1))
        {
          bool pressed = !(curr & 1);

          if (layers[layer].second.on_hold_bound())
          // if (layers[layer].second.Bound(keys[i * 8 + j], fex::Operation::HOLD))
          {
            if (pressed)
            {
              printf("fex: pressed??\n");
              timeouts[i * 8 + j] = xTaskGetTickCount();
            }
            else
            {
              if (timeouts[i * 8 + j] != -1 && now - timeouts[i * 8 + j] < hold)
              {

                layers[layer].second.Enqueue(keys[i * 8 + j], fex::Operation::PRESS, fex::BoundActionEnqueue::DO, xMessageQueue);
                layers[layer].second.Enqueue(keys[i * 8 + j], fex::Operation::PRESS, fex::BoundActionEnqueue::UNDO, xMessageQueue);
              }
              else
              {
                layers[layer].second.Enqueue(keys[i * 8 + j], fex::Operation::HOLD, fex::BoundActionEnqueue::UNDO, xMessageQueue);
              }
              timeouts[i * 8 + j] = -1;
            }
          }
          else
          {
            fex::BoundActionEnqueue bae = (pressed) ? fex::BoundActionEnqueue::DO : fex::BoundActionEnqueue::UNDO;
            layers[layer].second.Enqueue(keys[i * 8 + j], fex::Operation::PRESS, bae, xMessageQueue);
          }
        }

        prev = (prev >> 1);
        curr = (curr >> 1);
      }
    }

    memcpy(previous, current, 10);
  }
}

/*-----------------------------------------------------------*/

static void prvDrawDisplaysTask(void *pvParameters)
{
  printf("Starting Draw Displays Task...\n");
  TickType_t nextWake = xTaskGetTickCount();

  // TwoWire wire(i2c1, 6, 7);
  // Adafruit_SSD1306 display(SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, &wire);
  // display.setRotation(2);
  // display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  // Adafruit_SSD1306 display2(SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, &wire);
  // display2.setRotation(2);
  // display2.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // int i = 0;
  while (true)
  {
    xTaskDelayUntil(&nextWake, DRAW_DISPLAYS_TASK_PERIOD);

    // xSemaphoreTake(xI2CMutex, portMAX_DELAY);
    // // Clear the buffer.
    // display.clearDisplay();
    // display2.clearDisplay();

    // // Display Text
    // display.setTextSize(1);
    // display.setTextColor(WHITE);
    // display.setCursor(0, SSD1306_LCDHEIGHT / 3);
    // display.println(layers[layer].first.c_str());
    // display.display();
    // display2.setTextSize(1);
    // display2.setTextColor(WHITE);
    // display2.setCursor(0, 0);
    // display2.println("Goodbye world!!");
    // display2.println(std::to_string(i * 2).c_str());
    // display2.println(parse_status.c_str());
    // display2.display();
    // xSemaphoreGive(xI2CMutex);
    // i++;
  }
}

/*-----------------------------------------------------------*/

// TODO(fex): I'd like to delete this entirely
// and/or make a more generic "process queue" function
static void send_hid_report()
{
  if (!tud_hid_ready())
  {
    printf("core %d: hid not ready\n", get_core_num());
    return;
  }

  if (!hid_send_complete)
  {
    return;
  }

  static uint8_t keycode[KEY_ROLL_OVER] = {0};
  static uint8_t modifer = 0;
  static uint8_t mouse_buttons = 0;

  fex::QueueMessage msg;
  if (xQueueReceive(xMessageQueue, (void *)&msg, 0) != pdTRUE)
  {
    return;
  }

  if (msg.type == fex::MessageType::REBOOT)
  {
    watchdog_reboot(0, 0, 100);
    return;
  }

  if (msg.type == fex::MessageType::REBOOT_BOOTLOADER)
  {
    reset_usb_boot(0, 0);
    return;
  }

  if (msg.type == fex::MessageType::MOUSE_MOVE_UP_DOWN)
  {
    hid_send_complete = false;
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0, 0, msg.mouse_delta, 0, 0);
    return;
  }

  if (msg.type == fex::MessageType::MOUSE_MOVE_LEFT_RIGHT)
  {
    hid_send_complete = false;
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0, msg.mouse_delta, 0, 0, 0);
    return;
  }

  if (msg.type == fex::MessageType::MOUSE_SCROLL_UP_DOWN)
  {
    hid_send_complete = false;
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0, 0, 0, msg.mouse_delta, 0);
    return;
  }

  if (msg.type == fex::MessageType::MOUSE_MOVE_LEFT_RIGHT)
  {
    hid_send_complete = false;
    tud_hid_mouse_report(REPORT_ID_MOUSE, 0, 0, 0, 0, msg.mouse_delta);
    return;
  }

  if (msg.type == fex::MessageType::MOUSE_CLICK)
  {
    hid_send_complete = false;
    mouse_buttons = mouse_buttons | msg.mouse_click;
    tud_hid_mouse_report(REPORT_ID_MOUSE, mouse_buttons, 0, 0, 0, 0);
    return;
  }

  if (msg.type == fex::MessageType::MOUSE_RELEASE)
  {
    hid_send_complete = false;
    mouse_buttons = mouse_buttons & ~msg.mouse_click;
    tud_hid_mouse_report(REPORT_ID_MOUSE, mouse_buttons, 0, 0, 0, 0);
    return;
  }

  if (msg.type == fex::MessageType::LAYER_SWITCH)
  {
    layer = msg.layer;
    return;
  }

  if (msg.type == fex::MessageType::DELAY)
  {
    // TODO(fex): There is a weird bug where state gets messed
    // up when delays overlap
    printf("Delaying for %dms\n", msg.delay);
    vTaskDelay(pdMS_TO_TICKS(msg.delay));
    return;
  }

  if (msg.type == fex::MessageType::PRESS)
  {
    for (uint8_t i = 0; i < msg.length; i++)
    {
      uint8_t code = msg.codes[i];
      switch (code)
      {
      case HID_KEY_CONTROL_LEFT:
        modifer = modifer | KEYBOARD_MODIFIER_LEFTCTRL;
        break;
      case HID_KEY_SHIFT_LEFT:
        modifer = modifer | KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case HID_KEY_ALT_LEFT:
        modifer = modifer | KEYBOARD_MODIFIER_LEFTALT;
        break;
      case HID_KEY_GUI_LEFT:
        modifer = modifer | KEYBOARD_MODIFIER_LEFTGUI;
        break;
      case HID_KEY_CONTROL_RIGHT:
        modifer = modifer | KEYBOARD_MODIFIER_RIGHTCTRL;
        break;
      case HID_KEY_SHIFT_RIGHT:
        modifer = modifer | KEYBOARD_MODIFIER_RIGHTSHIFT;
        break;
      case HID_KEY_ALT_RIGHT:
        modifer = modifer | KEYBOARD_MODIFIER_RIGHTALT;
        break;
      case HID_KEY_GUI_RIGHT:
        modifer = modifer | KEYBOARD_MODIFIER_RIGHTGUI;
        break;

      default:
      {
        uint8_t index = 0;
        while (index < KEY_ROLL_OVER)
        {
          if (keycode[index] == 0)
          {
            break;
          }
          index++;
        }

        if (index == KEY_ROLL_OVER)
        {
          // TODO(fex): if next == KEY_ROLL_OVER; report error message
        }

        keycode[index] = code;
        break;
      }
      }
    }
  }

  if (msg.type == fex::MessageType::RELEASE)
  {
    for (uint8_t i = 0; i < msg.length; i++)
    {
      uint8_t code = msg.codes[i];
      switch (code)
      {
      case HID_KEY_CONTROL_LEFT:
        modifer = modifer & ~KEYBOARD_MODIFIER_LEFTCTRL;
        break;
      case HID_KEY_SHIFT_LEFT:
        modifer = modifer & ~KEYBOARD_MODIFIER_LEFTSHIFT;
        break;
      case HID_KEY_ALT_LEFT:
        modifer = modifer & ~KEYBOARD_MODIFIER_LEFTALT;
        break;
      case HID_KEY_GUI_LEFT:
        modifer = modifer & ~KEYBOARD_MODIFIER_LEFTGUI;
        break;
      case HID_KEY_CONTROL_RIGHT:
        modifer = modifer & ~KEYBOARD_MODIFIER_RIGHTCTRL;
        break;
      case HID_KEY_SHIFT_RIGHT:
        modifer = modifer & ~KEYBOARD_MODIFIER_RIGHTSHIFT;
        break;
      case HID_KEY_ALT_RIGHT:
        modifer = modifer & ~KEYBOARD_MODIFIER_RIGHTALT;
        break;
      case HID_KEY_GUI_RIGHT:
        modifer = modifer & ~KEYBOARD_MODIFIER_RIGHTGUI;
        break;

      default:
      {
        for (int8_t i = 0; i < KEY_ROLL_OVER; i++)
        {
          if (keycode[i] == code)
          {
            keycode[i] = 0;
            break;
          }
        }
      }
      }
    }
  }

  hid_send_complete = false;
  tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifer, keycode);

  printf("%d: ", modifer);
  for (uint8_t i = 0; i < KEY_ROLL_OVER; i++)
  {
    printf("%d, ", keycode[i]);
  }
  printf("\n");
}

static void prvUsbHidTask(void *pvParameters)
{
  printf("Starting USB HID Task...\n");
  TickType_t nextWake = xTaskGetTickCount();

  while (1)
  {
    xTaskDelayUntil(&nextWake, USB_HID_TASK_PERIOD);

    uint32_t const btn = board_button_read();

    // printf("core %d: hid task (%d)\n", get_core_num(), btn);

    // Remote wakeup
    if (tud_suspended() && btn)
    {
      // Wake up host if we are in suspend mode
      // and REMOTE_WAKEUP feature is enabled by host
      tud_remote_wakeup();
    }
    else
    {
      send_hid_report();
    }
  }
}

/*-----------------------------------------------------------*/

static void prvBlinkTask(void *pvParameters)
{
  printf("Starting Blink Task...\n");
  TickType_t nextWake = xTaskGetTickCount();

  while (true)
  {
    xTaskDelayUntil(&nextWake, BLINK_TASK_PERIOD);
    gpio_xor_mask(1u << BLINK_TASK_LED);
    printf("core %d: Blinking\n", get_core_num());
  }
}

/*-----------------------------------------------------------*/

void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint8_t len)
{
  hid_send_complete = true;
}

/*-----------------------------------------------------------*/

static void prvHardwareInit(void)
{
  stdio_init_all();
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);
  gpio_put(PICO_DEFAULT_LED_PIN, !PICO_DEFAULT_LED_PIN_INVERTED);
  board_init();
}
