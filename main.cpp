#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <math.h>     //for sine funcs, allat
#include <vector>     //for std::vector
#include <functional> //for std::function

// screen pins, display initialization
#define TFT_SCLK 18 // SCL -> GPIO 18
#define TFT_MOSI 23 // SDA -> GPIO 19
#define TFT_CS 5    // CS -> GPIO 5
#define TFT_RST 16  // RES -> GPIO 23
#define TFT_DC 17   // DC -> GPIO 16

#define RGB565(r, g, b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// generic pin definitions
#define leftenc_pin 22
#define rightenc_pin 19
#define buttonenc_pin 32
#define test_R_LED_pin 13
#define test_G_LED_pin 33
#define test_B_LED_pin 27

int frequency = 500;
int step1 = 100;

byte enc_rotatedflag = 0;
byte enc_lastposition = 0;      // 0-none;1-left on, right off; 2-left off, right on;3-right on, left on;
byte enclastbuttonposition = 0; // 0-none;1-pressed
byte encmoved = 0;              // 0-no movement; 1-to the left; 2-to the right
byte encbuttonpressed = 0;      // 0-button wasn't pressed in this itteration; 1-button pressed, activates for one itteration
// apps class, variables, funcs, etc.
int lastcreatedappID = 0;
uint16_t openedappID = 0;
uint16_t desiredopenedappID = 0;
uint16_t screen_fps = 15; // frames per second
uint16_t default_bg_color = ST7735_BLACK;
unsigned long lastmillis = 0;

class application;
std::vector<application> apps = {};
class application
{
public:
  String Name;
  int ID;
  std::function<void()> _setup;
  std::function<void(byte)> _loop;

  application()
  {
    Name = "Untitled App";
    ID = lastcreatedappID;
    lastcreatedappID++;
    apps.push_back(*this);
  }
  application(String name_, std::function<void()> setup_, std::function<void(byte)> loop_)
  {
    Name = name_;
    ID = lastcreatedappID;
    lastcreatedappID++;
    _setup = setup_;
    _loop = loop_;
    apps.push_back(*this);
  }

  void app_setup()
  {
    _setup();
  }
  void app_loop(byte shutdown = 0)
  {
    _loop(shutdown);
  }
};

/*void push_back_app(application to_push_app) {
  apps.push_back(to_push_app);
}*/
// NO APPS SHOULD BE DECLARED BEFORE THIS LINE, ONLY AFTER;

void open_app(uint16_t app_id)
{
  application openedapp = apps[openedappID];
  openedapp.app_loop(1); // shutting down the app;
  application newopenedapp = apps[app_id];
  desiredopenedappID = app_id;
  openedappID = app_id;
}

// MAIN MENU
uint16_t selected_mainmenu = 1;
uint16_t selected_color_mainmenu = 0xfe02; // orangish
uint16_t default_color_mainmenu = 0xae5f;  // pale bluish
void menu_loop(byte shutdown = 0)
{
  if (shutdown)
  {
    Serial.println("main menu shutting down");
    return;
  }
    ////Serial.println("main menu r");
  if (encmoved == 1)
  {
    selected_mainmenu = selected_mainmenu - 1;
    if (selected_mainmenu < 1) selected_mainmenu = apps.size()-1;
  }
  else if (encmoved == 2)
  {
    selected_mainmenu = selected_mainmenu + 1;
    if (selected_mainmenu > apps.size()-1) selected_mainmenu = 1;

  }
  if (abs(long(millis() - lastmillis)) >= long(1000 / screen_fps))
  {
    Serial.print("selected_mainmenu:");
    Serial.println(selected_mainmenu);
    lastmillis = millis();
    tft.fillScreen(default_bg_color);
    
    // this runs either at screen_fps frequency or does it's best to reach it, if the value is too high to handle
    byte offset_x = 5; // offset of the app list from (0;0)
    byte offset_y = 5; // offset of the app list from (0;0)
    byte spacing = 5;  // spacing between apps in the list, on Y axis
    Serial.print("apps size:"); Serial.println(apps.size());
    for (int i = 1; i < apps.size(); i++)
    {
      //application app = apps[i];
      byte text_size = 6;
      tft.setCursor(offset_x, offset_y + (spacing + text_size) * i);
      tft.setTextSize(1);
      if (selected_mainmenu == i)
      {
        tft.setTextColor(selected_color_mainmenu);
      }
      else
      {
        tft.setTextColor(default_color_mainmenu);
      }
      tft.print(i);
      tft.print(". ");
      tft.print(apps[i].Name);
    }
  }

  if (encbuttonpressed == 1)
  {
    desiredopenedappID = selected_mainmenu;
    Serial.println("mainmenu:button pressed");
  }
}
void menu_setup() {}
application main_menu(String("main_menu"), menu_setup, menu_loop);

// APP1:Test
// this app will manipulate RGB LED
uint8_t test_red_value = 0;   // 0-527
uint8_t test_green_value = 0; // 0-527
uint8_t test_blue_value = 0;  // 0-527
uint8_t step = 20;
uint8_t test_selected = 0;             // selected item index
uint8_t test_changing = 0;             // defines whether current selected is being changed
uint16_t test_changing_color = 0xffe0; // color when changing

void test_setup() {}
uint16_t test_checkcolor(int i)
{
  uint16_t color;
  if (test_selected == i)
  {
    if (test_changing)
    {
      color = test_changing_color;
    }
    else
    {
      color = selected_color_mainmenu;
    }
  }
  else
  {
    color = default_color_mainmenu;
  }
  return color;
}

void test_loop(byte shutdown = 0)
{
  Serial.println("test running");
  if (encbuttonpressed == 1)
  {
    if (test_selected == 0)
    {
      desiredopenedappID = 0;
      return;
    }
    else
    {
      test_changing = !test_changing;
    }
  }
  if (encmoved == 1)
  {
    if (test_changing)
    {
      switch (test_selected)
      {
      case 0:
        desiredopenedappID = 0;
        return;
      case 1:
        test_red_value = test_red_value - step;
        break;
      case 2:
        test_green_value = test_green_value - step;
        break;
      case 3:
        test_blue_value = test_blue_value - step;
        break;
      }
    }
    else
    {
      test_selected = test_selected - 1;
    }
  }
  else if (encmoved == 2)
  {
    if (test_changing)
    {
      switch (test_selected)
      {
      case 0:
        desiredopenedappID = 0;
        return;
      case 1:
        test_red_value = test_red_value + step;
        break;
      case 2:
        test_green_value = test_green_value + step;
        break;
      case 3:
        test_blue_value = test_blue_value + step;
        break;
      }
    }
    else
    {
      test_selected = test_selected + 1;
    }
  }
  if ((millis() - lastmillis) >= (1000 / screen_fps))
  {
    tft.fillScreen(default_bg_color);
    lastmillis = millis();
    byte offset_x = 5; // offset of the parameter list from (0;0)
    byte offset_y = 5; // offset of the parameter list from (0;0)
    byte spacing = 5;  // spacing between parameters in the list, on Y axis
    byte text_size = 6;
    for (int i = 0; i <= 3; i++)
    {
      switch (i) {
        case 0:
        tft.setCursor(offset_x, offset_y + (spacing + text_size) * i);
        tft.setTextColor(test_checkcolor(i));
        tft.print("Exit to main menu");
        break;
        case 1:
        tft.setCursor(offset_x, offset_y + (spacing + text_size) * i);
        tft.setTextColor(test_checkcolor(i));
        tft.print("Red:");
        tft.print(test_red_value);
        break;
        case 2:
        tft.setCursor(offset_x, offset_y + (spacing + text_size) * i);
        tft.setTextColor(test_checkcolor(i));
        tft.print("Green:");
        tft.print(test_green_value);
        break;
        case 3:
        tft.setCursor(offset_x, offset_y + (spacing + text_size) * i);
        tft.setTextColor(test_checkcolor(i));
        tft.print("Blue:");
        tft.print(test_blue_value);
        break;
      }
    }
  }
}

application test(String("Test"),test_setup,test_loop);

//RANDOM APP 
void random_loop(byte shutdown =0 ) {
  tft.fillScreen(ST7735_BLUE);
  if (encbuttonpressed) {
    desiredopenedappID = 0;
  }
}
void random_setup() {}
application randomapp(String("Random"),random_setup,random_loop);

void setup() {
  // initialization
  tft.initR(INITR_BLACKTAB);
  tft.fillScreen(ST7735_BLACK);
  pinMode(leftenc_pin, INPUT_PULLUP);
  pinMode(rightenc_pin, INPUT_PULLUP);
  pinMode(buttonenc_pin, INPUT_PULLUP);
  pinMode(test_R_LED_pin, OUTPUT);
  pinMode(test_G_LED_pin, OUTPUT);
  pinMode(test_B_LED_pin, OUTPUT);
  Serial.begin(115200);
  Serial.print("apps[0]:"); Serial.println(apps[0].Name);
  Serial.print("apps[1]:"); Serial.println(apps[1].Name);
  Serial.print("apps[2]:"); Serial.println(apps[2].Name);
}

void loop() {
    // encoder logic
    byte leftstate = digitalRead(leftenc_pin);
    byte rightstate = digitalRead(rightenc_pin);
    byte position = 0;
    if (leftstate && !rightstate)
    {
      position = 1;
    }
    else if (rightstate && !leftstate)
    {
      position = 2;
    }
    else if (rightstate && leftstate)
    {
      position = 3;
      if (enc_lastposition == 1)
      {
        frequency = frequency + step1;
        encmoved = 2;
        // tft.fillScreen(ST7735_BLACK);
        // tft.setCursor(20, 30);
        // tft.print(frequency);
        // tft.print("Hz");
      }
      else if (enc_lastposition == 2)
      {
        frequency = frequency - step1;
        encmoved = 1;
        // tft.fillScreen(ST7735_BLACK);
        // tft.setCursor(20, 30);
        // tft.print(frequency);
        // tft.print("Hz");
      }
    }
    byte encbutton_reading = digitalRead(buttonenc_pin);
    if (enclastbuttonposition == 1 && !encbutton_reading)
    {
      encbuttonpressed = 1;
    }

    // apps logic
    if (desiredopenedappID == openedappID)
    {
      application openedapp = apps[openedappID];
      openedapp.app_loop();
    }
    else
    {
      open_app(desiredopenedappID);
      Serial.print("Opening app:");Serial.println(desiredopenedappID);
    }

    // reseting values
    enc_lastposition = position;
    encmoved = 0;
    encbuttonpressed = 0;
    enclastbuttonposition = encbutton_reading;
    //lastmillis = millis();
  }
