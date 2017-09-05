/* -*- C++ -*- */
/********************
Sept 2014 ~ Sept. 2016 Rui Azevedo - ruihfazevedo(@rrob@)gmail.com
creative commons license 3.0: Attribution-ShareAlike CC BY-SA
This software is furnished "as is", without technical support, and with no
warranty, express or implied, as to its usefulness for any purpose.

Thread Safe: No
Extensible: Yes

menu library objects
definitions and enumerations

www.r-site.net

thanks to sphh (https://github.com/sphh)
for correcting unsigned values validation

***/

#ifndef RSITE_ARDUINO_MENU_SYSTEM
  #define RSITE_ARDUINO_MENU_SYSTEM
  #include <Arduino.h>
  #if defined(DEBUG) //!defined(ArduinoStream_h)
    #include <Streaming.h>
  #endif
  #include "menuBase.h"
  #include "shadows.h"
  //#include "dyn.h"

  namespace Menu {

    static const char* numericChars="0123456789.";

    #define _MAX(a,b) (((a)>(b))?(a):(b))
    //#if defined(ESP8266)
    //#if !defined(endl)
    // #if !defined(ARDUINO_STREAMING) || !defined(ArduinoStream_h)
    //   #define endl "\r\n"
    // #endif
    // Streams
    //////////////////////////////////////////////////////////////////////////
    #ifdef DEBUG
    //template<typename T> inline Print& operator<<(Print& o, T t) {o.print(t);return o;}
    Print& operator<<(Print& o,prompt const &p);
    inline String& operator<<(String &s,prompt &p);
    //template<typename T> HardwareSerial& operator<<(HardwareSerial& o,T t) {o.print(t);return o;}
    //template<typename T> inline menuOut& operator<<(menuOut& o,const T x) {return o.operator<<(x);}
    #endif

    // Menu objects and data
    //////////////////////////////////////////////////////////////////////////

    enum classes {noClass=0,promptClass,textFieldClass,fieldClass,toggleClass,selectClass,chooseClass,valueClass,menuClass};

    class prompt {
      friend class navNode;
      friend class menuOut;
      protected:
      public:
        constMEM promptShadow* shadow;//constant read-only data (PROGMEM)
      public:
        status enabled=enabledStatus;//ignore enter if false
        bool dirty=true;//needs to be  redrawn
        inline uint16_t hash() const {
          int tmp=(int)this;
          return ((uint16_t*)&tmp)[sizeof(this)/sizeof(uint16_t)-1];
        }
        virtual classes type() const {return promptClass;}
        inline prompt(constMEM promptShadow& shadow):shadow(&shadow) {}
        // inline prompt(constMEM char* t,action a=doNothing,eventMask e=noEvent,styles s=noStyle,systemStyles ss=_noStyle)
        //   :shadow(&promptShadow(t,a,e,s,ss)) {}
        inline void enable() {enabled=enabledStatus;}
        inline void disable() {enabled=disabledStatus;}
        inline const char* getText() const {return shadow->getText();}
        inline const systemStyles sysStyles() const {return shadow->_sysStyles();}
        inline const eventMask events() const {return shadow->_events();}
        inline styles style() const {return shadow->_style();}
        inline bool canNav() const {return sysStyles()&_canNav;}//can receive navigation focus and process keys
        inline bool isMenu() const {return sysStyles()&_menuData;}//has menu data list and can be a navNode target
        inline bool isVariant() const {return sysStyles()&_isVariant;}//a menu as an enumerated field, connected to a variable value
        inline bool parentDraw() const {return sysStyles()&_parentDraw;}//a menu as an enumerated field, connected to a variable value
        virtual idx_t printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len);//raw print to output device
        virtual bool changed(const navNode &nav,const menuOut& out,bool sub=true) {return dirty;}
        //this is the system version of enter handler, its used by elements like toggle
        virtual result sysHandler(SYS_FUNC_PARAMS) {return proceed;}
        inline result operator()(FUNC_PARAMS) const {return (*shadow)(FUNC_VALUES);}
        idx_t printRaw(menuOut& out,idx_t len) const;
        virtual prompt* seek(idx_t* uri,idx_t len) {return len?NULL:this;}
        virtual bool async(const char *uri,navRoot& root,idx_t lvl) {
          return ((!*uri)||(uri[0]=='/'&&!uri[1]));
        }

        //some functions to use on htmlFmt
        // for enumerations:
        virtual idx_t selected() const {return 0;}
        #ifdef DEBUG
          virtual void printValue(menuOut&) const {}
          virtual void printHigh(menuOut&) const {}
          virtual void printLow(menuOut&) const {}
        #endif

    };

    //--------------------------------------------------------------------------
    // can receive navigation focus and process keys
    // this is the minimal candidate for navRoot::navFocus
    class navTarget:public prompt {
      public:
        navTarget(constMEM promptShadow& shadow):prompt(shadow) {}
        // navTarget(constMEM char* t,action a=doNothing,eventMask e=noEvent,styles s=noStyle,systemStyles ss=_noStyle)
        //   :prompt(t,a,e,s,ss) {}
        //bool canNav() const override {return true;}
        virtual void parseInput(navNode& nav,Stream& in);
        virtual void doNav(navNode& nav,navCmd cmd);
    };

    //--------------------------------------------------------------------------
    class textField:public navTarget {
    public:
      //char* text;
      // int hash=0;//not implemented yet
      bool charEdit=false;
      bool edited=false;
      idx_t cursor=0;
      textField(constMEM textFieldShadow& shadow):navTarget(shadow) {}
      inline char* buffer() const {return ((textFieldShadow*)shadow)->_buffer();}
      inline idx_t sz() const {return ((textFieldShadow*)shadow)->_sz();}
      const char* validator(int i) {return ((textFieldShadow*)shadow)->operator[](i%sz());}
      void doNav(navNode& nav,navCmd cmd) override;
      idx_t printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) override;
    };

    //--------------------------------------------------------------------------
    //using this fieldBase saves 418 flash bytes per field template instantiation
    class fieldBase:public navTarget {
      public:
        bool tunning=false;
        fieldBase(constMEM promptShadow& shadow):navTarget(shadow) {}
        virtual classes type() const {return fieldClass;}
        bool async(const char *uri,navRoot& root,idx_t lvl) override;
        inline const char* units() {return ((fieldBaseShadow*)shadow)->_units();}
        void doNav(navNode& nav,navCmd cmd) override;
        virtual bool canTune()=0;
        virtual void constrainField()=0;
        virtual void stepit(int increment)=0;
        virtual idx_t printReflex(menuOut& o) const =0;
        idx_t printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) override;
    };
    //--------------------------------------------------------------------------
    template<typename T>
    class menuField:public fieldBase {
      public:
        T reflex;
        menuField(constMEM menuFieldShadow<T> & shadow):fieldBase(shadow) {}
        bool canTune() override {return !!tune();}
        void constrainField() override {target() = constrain(target(), low(), high());}
        idx_t printReflex(menuOut& o) const override;
        void parseInput(navNode& nav,Stream& in) override;
        idx_t printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) override;
        inline T& target() const {return ((menuFieldShadow<T>*)shadow)->target();}
        inline T getTypeValue(const T* from) const {return ((menuFieldShadow<T>*)shadow)->getTypeValue(from);}
        inline T low() const {return  ((menuFieldShadow<T>*)shadow)->_low();}
        inline T high() const {return ((menuFieldShadow<T>*)shadow)->_high();}
        inline T step() const {return ((menuFieldShadow<T>*)shadow)->_step();}
        inline T tune() const {return ((menuFieldShadow<T>*)shadow)->_tune();}
        bool changed(const navNode &nav,const menuOut& out,bool sub=true) override {
          return dirty||(reflex!=target());
        }
        #ifdef DEBUG
          void printValue(menuOut& o) const override;
          void printHigh(menuOut& o) const override;
          void printLow(menuOut& o) const override;
        #endif
        void stepit(int increment) override {
          int sign = increment*(options->invertFieldKeys?-1:1);
          T thisstep = tunning?tune():step();
          dirty=true;
          if (sign > 0) {
            if ((high()-target()) < thisstep) {
              if (style()&wrapStyle) {
                target() = low();
              } else {
                target() = high();
              }
            } else {
              target() += thisstep;
            }
          } else {
            if ((target()-low()) < thisstep) {
              if (style()&wrapStyle) {
                target() = high();
              } else {
                target() = low();
              }
            } else {
              target() -= thisstep;
            }
          }
        }
      };

    //--------------------------------------------------------------------------
    template<typename T>
    class menuValue:public prompt {
      public:
        menuValue(constMEM menuValueShadow<T>& shadow):prompt(shadow) {}
        #ifdef DEBUG
        bool changed(const navNode &nav,const menuOut& out,bool sub=true) override {return false;}
        #endif
        //inline T getTypeValue(const T* from) const {return &((menuValueShadow<T>*)shadow)->getTypeValue(from);}
        inline T target() const {return ((menuValueShadow<T>*)shadow)->target();}
        virtual classes type() const {return valueClass;}
    };

    //--------------------------------------------------------------------------
    class menuNode:public navTarget {
      public:
        menuNode(constMEM menuNodeShadow& s):navTarget(s) {}
        virtual classes type() const {return menuClass;}
        inline prompt& operator[](idx_t i) const {return ((menuNodeShadow*)shadow)->operator[](i);}
        bool changed(const navNode &nav,const menuOut& out,bool sub=true) override;
        inline idx_t sz() const {return ((menuNodeShadow*)shadow)->_sz();}
        inline prompt* const* data() const {return ((menuNodeShadow*)shadow)->_data();}
        prompt* seek(idx_t* uri,idx_t len) override;
        bool async(const char *uri,navRoot& root,idx_t lvl=0) override;
    };

    //--------------------------------------------------------------------------
    class menu:public menuNode {
      public:
        menu(constMEM menuNodeShadow& shadow):menuNode(shadow) {}
    };

    //--------------------------------------------------------------------------
    class menuVariantBase:public menuNode {
      public:
        menuVariantBase(constMEM menuNodeShadow& s):menuNode(s) {}
        virtual idx_t sync()=0;
        virtual idx_t sync(idx_t i)=0;
        idx_t printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) override;
        idx_t togglePrintTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len);
        void doNav(navNode& nav,navCmd cmd) override;
    };
    template<typename T>
    class menuVariant:public menuVariantBase {
      public:
        idx_t reflex;
        menuVariant(constMEM menuNodeShadow& s):menuVariantBase(s) {}
        idx_t sync() override {
          //menuVariantShadow<T>& s=*(menuVariantShadow<T>*)shadow;
          for(idx_t i=0;i<sz();i++) {
            if (((menuValue<T>*)&operator[](i))->target()==target()) {
              dirty=true;
              return i;
            }
          }
          #ifdef DEBUG
          Serial.print(F("value out of range "));
          Serial.println(target());Serial.flush();
          assert(false);
          #endif
          return -1;
        }
        idx_t sync(idx_t i) override {
          //menuVariantShadow<T>& s=*(menuVariantShadow<T>*)shadow;
          #ifdef DEBUG
          if (!(i>=0&&i<sz())){
            print_P(Serial,getText());
            Serial.print(F(" : value out of range "));
            Serial.println(i);
          }
          assert(i>=0&&i<sz());
          #endif
          reflex=i;
          target()=((menuValue<T>*)&operator[](i))->target();
          return i;
        }
        inline T& target() const {return ((menuVariantShadow<T>*)shadow)->target();}
        bool changed(const navNode &nav,const menuOut& out,bool sub=true) override;
        //void parseInput(navNode& nav,Stream& in) override;
        //idx_t printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) override;
        virtual idx_t selected() const {return reflex;}
    };

    template<typename T>//-------------------------------------------
    class select:public menuVariant<T> {
      public:
        select(constMEM menuNodeShadow& s):menuVariant<T>(s) {}
        virtual classes type() const {return selectClass;}
    };

    template<typename T>//-------------------------------------------
    class toggle:public menuVariant<T> {
      public:
        toggle(constMEM menuNodeShadow& s):menuVariant<T>(s) {}
        virtual classes type() const {return toggleClass;}
        idx_t printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) override;
        //bool canNav() const override {return false;}//can receive navigation focus and process keys
        result sysHandler(SYS_FUNC_PARAMS) override {
          switch(event) {
              case activateEvent: {
              //menuNodeShadow& s=*(menuNodeShadow*)prompt::shadow;
              idx_t at=menuVariant<T>::sync();
              assert(at!=-1);
              at++;
              if (at>=menuNode::sz()) at=0;
              menuVariant<T>::sync(at);
              prompt::dirty=true;
              (menuNode::operator[](at))(FUNC_VALUES);
            }
            default:
              return proceed;
          }
          //return proceed;
        }
    };

    template<typename T>//-------------------------------------------
    class choose:public menuVariant<T> {
      public:
        choose(constMEM menuNodeShadow& s):menuVariant<T>(s) {}
        virtual classes type() const {return chooseClass;}
        result sysHandler(SYS_FUNC_PARAMS) override;
        bool changed(const navNode &nav,const menuOut& out,bool sub=true) override {
          return menuVariant<T>::changed(nav,out)||menuNode::changed(nav,out);
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    // Output
    ////////////////////////////////////////////////////////////////////////////

    //navigation panels (min 1) describe output dimensions (in characters)
    struct panel {
      idx_t x,y,w,h;
      inline idx_t maxX() const {return x+w;}
      inline idx_t maxY() const {return y+h;}
    };

    class panelsList {
      public:
        const panel* panels;
        navNode** nodes;//TODO use navNode here!
        const idx_t sz;
        idx_t cur=0;
        panelsList(const panel p[],navNode* nodes[],idx_t sz):panels(p),nodes(nodes),sz(sz) {
          reset();
        }
        void reset(idx_t from=0) {
          for(int n=from;n<sz;n++) nodes[n]=NULL;
        }
        inline const panel operator[](idx_t i) const {
          assert(i<sz);
          #ifdef USING_PGM
            panel tmp;
            memcpy_P(&tmp, &panels[i], sizeof(panel));
            return tmp;
          #else
            return panels[i];
          #endif
        }
        idx_t maxX() const {
          idx_t r=0;
          for(int n=0;n<sz;n++) r=_MAX(operator[](n).maxX(),r);
          return r;
        }
        idx_t maxY() const {
          idx_t r=0;
          for(int n=0;n<sz;n++) r=_MAX(operator[](n).maxY(),r);
          return r;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // base for all menu output devices
    class menuOut:public Print {
      public:
        idx_t* tops;
        panelsList& panels;
        idx_t lastSel=-1;
        //TODO: turn this bool's into bitfield flags
        enum styles {none=0<<0,redraw=1<<0,minimalRedraw=1<<1, drawNumIndex=1<<2, usePreview=1<<3, expandEnums=1<<4} style;
        enum fmtParts {fmtPanel,fmtTitle,fmtBody,fmtOp,fmtIdx,fmtCursor,fmtOpBody,fmtPreview,fmtPrompt,fmtField,fmtToggle,fmtSelect,fmtChoose,fmtUnit};

        /*bool redraw=false;//redraw all menu every cycle, some display drivers require it
        bool minimalRedraw=true;//redraw only changed options (avoids flicking on LCDS), not good for Serial
        bool drawNumIndex=false;
        bool usePreview=false;*/
        menuNode* drawn=NULL;
        menuOut(idx_t *topsList,panelsList &p,styles os=minimalRedraw)
          :tops(topsList),panels(p),style(os) {}
        inline idx_t maxX(idx_t i=0) const {return panels[i].w;}
        inline idx_t maxY(idx_t i=0) const {return panels[i].h;}
        inline idx_t& top(navNode& nav) const;
        idx_t printRaw(const char* at,idx_t len);
        #ifdef DEBUG
        virtual menuOut& operator<<(prompt const &p);
        #ifdef ESP8266
        template<typename T> menuOut& operator<<(T o) {(*(Print*)this)<<(o);return *this;}
        #endif
        #endif
        virtual menuOut& fill(
          int x1, int y1, int x2, int y2,char ch=' ',
          colorDefs color=bgColor,
          bool selected=false,
          status stat=enabledStatus,
          bool edit=false
        ) {return *this;}
        void clearChanged(navNode &nav);//clean up changed flags after everyone printed!
        void previewMenu(navRoot& root,menuNode& menu,idx_t panelNr);//draw a preview on a panel
        void printMenu(navNode &nav);//print menus and previews on panels
        virtual void clearLine(idx_t ln,idx_t panelNr=0,colorDefs color=bgColor,bool selected=false,status stat=enabledStatus,bool edit=false)=0;
        virtual void clear()=0;
        virtual void clear(idx_t panelNr)=0;
        virtual void setCursor(idx_t x,idx_t y,idx_t panelNr=0)=0;
        virtual void setColor(colorDefs c,bool selected=false,status s=enabledStatus,bool edit=false) {}
        virtual void drawCursor(idx_t ln,bool selected,status stat,bool edit=false,idx_t panelNr=0) {
          setColor(cursorColor, selected, stat,edit);
          write(selected?(stat==disabledStatus?options->disabledCursor:options->selectedCursor):' ');
        }
        void doNav(navCmd cmd,navNode &nav);
        virtual result fmtStart(fmtParts part,navNode &nav,idx_t idx=-1) {return proceed;}
        virtual result fmtEnd(fmtParts part,navNode &nav,idx_t idx=-1) {return proceed;}
        virtual void startCursor(bool charEdit) {write(charEdit?">":"[");}
        virtual void endCursor(bool charEdit) {write(charEdit?"<":"]");}
      protected:
        void printMenu(navNode &nav,idx_t panelNr);
    };

    //for devices that can position a print cursor (like LCD's)
    class cursorOut:public menuOut {
    public:
      cursorOut(idx_t *topsList,panelsList &p,styles os=minimalRedraw)
        :menuOut(topsList,p,os) {}
      void clearLine(idx_t ln,idx_t panelNr=0,colorDefs color=bgColor,bool selected=false,status stat=enabledStatus,bool edit=false) override {
        setCursor(0,ln,panelNr);
        for(int n=0;n<maxX();n++) print(' ');
        setCursor(0,ln,panelNr);
      }
      void clear(idx_t panelNr) override {
        const panel p=panels[panelNr];
        fill(p.x,p.y,p.x+p.w-1,p.y+p.h-1);
        setCursor(0,0,panelNr);
        panels.nodes[panelNr]=NULL;
      }
      menuOut& fill(
        int x1, int y1, int x2, int y2,char ch=' ',
        colorDefs color=bgColor,
        bool selected=false,
        status stat=enabledStatus,
        bool edit=false
      ) override {
        for(int r=y1;r<=y2;r++) {
          setCursor(x1,r);
          for(int c=x1;c<=x2;c++)
            write(ch);
        }
        return *this;
      }
    };

    class gfxOut:public menuOut {
      public:
        idx_t resX=1;
        idx_t resY=1;
        idx_t fontMarginY=1;//in pixels, compensate vertical font alignment
        gfxOut(idx_t rx,idx_t ry,idx_t* t,panelsList &p,menuOut::styles st=menuOut::minimalRedraw)
          :menuOut(t,p,st),resX(rx),resY(ry) {}
        //void printMenu(navNode &nav) override;
    };

    //list of output devices
    //this allows parallel navigation on multiple devices
    class outputsList {
      public:
        int cnt=1;
        menuOut* const* outs;
        outputsList(menuOut* const o[],int n):cnt(n),outs(o) {}
        menuOut& operator[](idx_t i) {
          assert(i<cnt);
          return *(menuOut*)memPtr(outs[i]);
        }
        void printMenu(navNode& nav) const {
          for(int n=0;n<cnt;n++)
            ((menuOut*)memPtr(outs[n]))->printMenu(nav);
          clearChanged(nav);
        }
        //void alert(char *msg) const {for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->alert(msg);}
        void clearLine(idx_t ln,idx_t panelNr=0,colorDefs color=bgColor,bool selected=false,status stat=enabledStatus) const {
          for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->clearLine(ln,panelNr,color,selected,stat);
        }
        void clearChanged(navNode& nav) const {
          for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->clearChanged(nav);
        }
        void clear() {for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->clear();}
        void setCursor(idx_t x,idx_t y) {
          for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->setCursor(x,y);
        }
        void setColor(colorDefs c,bool selected=false,status s=enabledStatus) {
          for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->setColor(c,selected,s);
        }
        void drawCursor(idx_t ln,bool selected,status stat,idx_t panelNr=0) {
          for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->drawCursor(ln,selected,stat,panelNr);
        }
        void doNav(navCmd cmd,class navNode &nav) {for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->doNav(cmd,nav);}
        result idle(idleFunc f,idleEvent e) {
          for(int n=0;n<cnt;n++) {
            menuOut& o=*((menuOut*)memPtr(outs[n]));
            switch(e) {
              case idleStart:
                if ((*f)(o,e)==proceed) {
                  if (!(o.style&menuOut::redraw)) {
                    //o.clear();//reset the coordinates and colors
                    result r=(*f)(o,idling);
                    if (r==quit) return r;
                  }
                } else return quit;
                break;
              case idling:
                if (o.style&menuOut::redraw) {
                  //o.clear();//reset the coordinates and colors
                  result r=(*f)(o,e);
                  if (r==quit) return r;
                }
                break;
              case idleEnd:
                result r=(*f)(o,e);
                if (r==quit) return r;
                break;
            }
          }
          return proceed;
        }
    };

    // input
    ////////////////////////////////////////////////////////////////////////////

    // Navigation
    //////////////////////////////////////////////////////////////////////////
    class navNode {
      friend class navRoot;
      protected:
        inline menuNodeShadow& shadow() const {return *(menuNodeShadow*)target->shadow;}
        inline void useMenu(menuNode &menu) {target=&menu;reset();}
      public:
        idx_t sel=0;
        menuNode* target;
        static navRoot* root;
        inline void reset() {sel=0;}
        inline idx_t sz() const {return target->sz();}
        inline prompt* const * data() const {return target->data();}
        inline prompt& selected() const {return *(prompt*)memPtr(data()[sel]);}
        inline bool wrap() const {return target->style()&wrapStyle;}
        /*inline result sysHandler(eventMask event, prompt &item, Stream &in, menuOut &out) const {
          return target->sysHandler(event,*this,item,in,out);
        }*/
        result event(eventMask e,idx_t i);//send event to item index i
        result event(eventMask e) {return event(e,sel);}//send event to current item
        result sysEvent(eventMask e,idx_t i);//send system event to item index i
        inline result sysEvent(eventMask e) {return sysEvent(e,sel);}//send event to current item
        navCmd navKeys(char ch);
        //inline void doNav(navCmd cmd) {target->doNav(*this,cmd);}
        navCmd doNavigation(navCmd cmd);//aux function
        inline bool changed(const menuOut& out) const {return target->changed(*this,out);}
        inline prompt& operator[](idx_t i) const {return target->operator[](i);}
        //inline void parseInput(Stream& in) {target->parseInput(*this,in);}
    };

    class navRoot {
      public:
        outputsList &out;
        Stream& in;
        navNode* path;
        const idx_t maxDepth=0;
        idx_t level=0;
        //bool suspended=false;
        bool showTitle=true;
        //bool sleeping=false;//when sleeping poll will simply return
        idleFunc idleTask=inaction;//to do when menu exits
        idleFunc sleepTask=NULL;//user task suspending menu
        navTarget* navFocus=NULL;
        navRoot(menuNode& root,navNode* path,idx_t d,Stream& in,outputsList &o)
          :out(o),in(in),path(path),maxDepth(d) {
            /*navFocus=&root;
            path[0].target=&root;*/
            useMenu(root);
            navNode::root=this;
            //for(int i=0;i<d;i++) path[i].root=this;
          }
        void useMenu(constMEM menuNode &menu) {
          navFocus=&menu;
          path[0].target=&menu;
          reset();
        }
        inline void reset() {
          level=0;
          path[0].sel=0;
        }
        inline navNode& node() const {return path[level];}
        inline menuNode& active() const {return *node().target;}
        inline prompt& selected() const {return active()[node().sel];}
        inline bool changed(const menuOut& out) const {return node().changed(out);}
        inline bool changed(idx_t n) const {return node().changed(out[n]);}
        //inline void parseInput(Stream& in) const {Serial<<"navRoot::parseInput"<<endl;;node().parseInput(in);}
        inline bool async(const char* at) {
          //Serial<<*(prompt*)&active()<<" navRoot::async "<<at<<endl;Serial.flush();
          //if (sleepTask) idleOff();
          navFocus=path[level].target;
          return active().async(at, *this, 0);
        }
        menuOut& printPath(menuOut& o) const {
          for(idx_t n=0;n<level;n++) {
            o.print('/');
            o.print(path[n].sel);
          }
          return o;
        }
        void printMenu() const {
          if ((active().sysStyles()&_parentDraw)&&level)
            out.printMenu(path[level-1]);
          else out.printMenu(node());
        }

        //async printMenu on arbitrary menuOut device
        void printMenu(menuOut& o) const {
          if ((active().sysStyles()&_parentDraw)&&level)
            o.printMenu(path[level-1]);
          else o.printMenu(node());
        }

        //menu IO - external iteration functions
        void doInput(Stream& in);
        inline void doInput(const char*in) {
          //Serial<<"do in start"<<endl;Serial.flush();
          StringStream inStr(in);
          while(inStr.available()) doInput(inStr);
          //Serial<<"do out end"<<endl;Serial.flush();
        }
        inline void doInput() {doInput(in);}
        inline void doOutput() {
          //Serial<<"do out start"<<endl;Serial.flush();
          if (!sleepTask) printMenu();
          else out.idle(sleepTask,idling);
          //Serial<<"do out end"<<endl;Serial.flush();
        }
        inline void poll() {doInput();doOutput();};//fire and forget mode
        void doNav(navCmd cmd);//fly by wire mode
        navCmd enter();//aux function
        navCmd exit();//aux function

        //enter idle mode ---------------------------------
        inline void idleOn(idleFunc task=inaction) {
          out.clear();
          sleepTask=task;
          active().dirty=true;
          out.idle(sleepTask,idleStart);
        }
        inline void idleOff() {
          out.idle(sleepTask,idleEnd);
          sleepTask=NULL;
          active().dirty=true;
          out.clear();
        }
        //inline void alert(char *msg,bool modal=true) {out.alert(msg);}
    };

    ////////////////////////////////////////////////////////////////////////
    template<typename T>
    idx_t menuField<T>::printReflex(menuOut& o) const {return o.print(reflex);}

    template<typename T>
    idx_t menuField<T>::printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) {
      //menuFieldShadow<T>& s=*(menuFieldShadow<T>*)shadow;
      reflex=target();
      return fieldBase::printTo(root,sel,out,idx,len);
    }

    template<typename T>
    void menuField<T>::parseInput(navNode& nav,Stream& in) {
      if (strchr(numericChars,in.peek())) {//a numeric value was entered
        if (options->numValueInput) {
          target()=(T)in.parseFloat();
          tunning=true;
          doNav(nav,enterCmd);
        } else doNav(nav,idxCmd);
      } else doNav(nav,nav.navKeys(in.read()));
    }

    #ifdef DEBUG
      template<typename T>
      void menuField<T>::printValue(menuOut& o) const {o.print(reflex);}
      template<typename T>
      void menuField<T>::printHigh(menuOut& o) const {o.print(high());}
      template<typename T>
      void menuField<T>::printLow(menuOut& o) const {o.print(low());}
    #endif

    template<typename T>
    idx_t toggle<T>::printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) {
      return menuVariantBase::togglePrintTo(root,sel,out,idx,len);
    }

/*    template<typename T>
    idx_t menuVariant<T>::printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len) {
      idx_t at=menuVariant<T>::sync(menuVariant<T>::sync());
      return menuVariantBase::printTo(root,sel,out,idx,len);
    }*/

    template<typename T>
    bool menuVariant<T>::changed(const navNode &nav,const menuOut& out,bool sub) {
      return dirty||((menuValue<T>*)&operator[](reflex))->target()!=target();
    }

    template<typename T>
    result choose<T>::sysHandler(SYS_FUNC_PARAMS) {
      switch(event) {
        case updateEvent:
        case enterEvent:
          nav.sel=menuVariant<T>::sync();
        default:
          return proceed;
      }
    }

    idx_t& menuOut::top(navNode& nav) const {return tops[nav.root->level];}

    #ifdef DEBUG
    inline String& operator<<(String &s,prompt &p) {return s+=p.getText();}
    #endif
  }//namespace Menu

#endif
