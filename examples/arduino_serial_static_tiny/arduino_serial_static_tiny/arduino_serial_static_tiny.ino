/*
This was just compiled.. not uploaded it to an attiny 13 yet...
the purpose was just to check memory contraints
upload at own risk ;)
*/

/* memory footprints

with idx_t=int8_t
DATA:    [=======   ]  65.6% (used 42 bytes from 64 bytes)
PROGRAM: [======    ]  57.8% (used 592 bytes from 1024 bytes)

with idx_t=int
DATA:    [========= ]  89.1% (used 57 bytes from 64 bytes)
PROGRAM: [=======   ]  68.6% (used 702 bytes from 1024 bytes)

with idx_t=int and using TinyNavPos (insignificant, 2 bytes per menu)
DATA:    [========= ]  85.9% (used 55 bytes from 64 bytes)
PROGRAM: [======    ]  64.5% (used 660 bytes from 1024 bytes)

2020.02.22 all enable/disable options
this is the mixin + CRTP composition model, i like it!
RAM:   [==        ]  18.4% (used 94 bytes from 512 bytes)
Flash: [===       ]  33.9% (used 2778 bytes from 8192 bytes)

2020.02.22 some endis options and all with actions
RAM:   [===       ]  27.5% (used 141 bytes from 512 bytes)
Flash: [====      ]  38.3% (used 3136 bytes from 8192 bytes)
*/

//ArduinoMenu main include for static menus
#include <staticMenu.h>
//input/output drivers --------------------------------------
#include <menu/IO/Arduino/serialIO.h>//include both serial in and out

#include <menu/fmt/fullText.h>
#include <menu/fmt/titleWrap.h>

#include <menu/comp/endis.h>
#include <menu/comp/Arduino/flashText.h>

using namespace Menu;

//menu output (Serial)
using Out=StaticMenuOut<
  FullPrinter::Part,//print title and items
  // TitleWrapFmt<>::Part,//put [] around menu title
  TextFmt::Part,//apply text formating
  SerialOut<decltype(Serial),Serial>::Part//raw output device
>;

Out out;//create output object (Serial)

//menu input --------------------------------------
SerialIn<decltype(Serial),Serial> in;//create input object (here serial)

//------------------------------
//menu action handlers
bool tog12();

bool op1_action() {
  Serial.println("Option 1 action called!");
  return true;//false would close the menu
}

bool op2_action() {
  Serial.println("Option 2 action called!");
  return true;
}

//////////////////////////////////////////////
// defining static menu

//string data on flash
extern const char op1_text[] PROGMEM="Op 1";
extern const char op2_text[] PROGMEM="Op 2";
extern const char op3_text[] PROGMEM="Op 3";
extern const char mainMenu_title[] PROGMEM="Main menu";

//the static menu structure
Item<
  StaticMenu<
    FlashText<decltype(mainMenu_title),&mainMenu_title>::Part<>,
    StaticData<
      Item<Action<op1_action>::Part,EnDis<true>::Part,FlashText<decltype(op1_text),&op1_text>::Part>,
      Item<Action<op2_action>::Part,EnDis<false>::Part,FlashText<decltype(op2_text),&op2_text>::Part>,
      Item<Action<tog12>::Part,FlashText<decltype(op3_text),&op3_text>::Part>
    >
  >::Part
> mainMenu;//create menu object

//navigation root ---------------------------------
StaticNavRoot<Nav<decltype(mainMenu),1>::Part> nav(mainMenu);

//menu action handlers implementation
bool tog12() {
  mainMenu.enable(!mainMenu.enabled(Path<0>().ref()),Path<0>().ref());
  mainMenu.enable(!mainMenu.enabled(Path<1>().ref()),Path<1>().ref());
  return true;
}

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println(F("AM5 serial example"));
  //now disabled at type definition
  // mainMenu.enable(false,Path<1>().ref());//disable second option
  nav.print(out);
}

void loop() {
  if (nav.doInput(in)) nav.print(out);
}