#include "menu.h"
using namespace Menu;

bool prompt::hasTitle(navNode& nav) const {return (nav.target->has(showTitle)||(nav.root->showTitle&&!nav.target->has(noTitle)));}

idx_t prompt::printRaw(menuOut& out,idx_t len) const {
  trace(MENU_DEBUG_OUT<<"prompt::printRaw"<<endl;print_P(MENU_DEBUG_OUT,getText(),len));
  return print_P(out,getText(),len);
}

Used prompt::printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t panelNr) {
  trace(MENU_DEBUG_OUT<<*(prompt*)this<<" prompt::printTo"<<endl);
  #ifdef MENU_FMT_WRAPS
    out.fmtStart(*this,menuOut::fmtPrompt,root.node(),idx);
  #endif
  idx_t r=printRaw(out,len);
  #ifdef MENU_FMT_WRAPS
    out.fmtEnd(*this,menuOut::fmtPrompt,root.node(),idx);
  #endif
  if (is((systemStyles)(_menuData|_parentDraw|_asPad))
    //&&((&((menuNode*)root.node().target)->operator[](idx))==this)
  ) {
    trace(MENU_DEBUG_OUT<<*(prompt*)this<<" some parentDraw or asPad menu... idx:"<<idx<<endl);
    // #ifdef MENU_FMT_WRAPS
    //   out.fmtStart(*this,menuOut::fmtBody,root.node(),idx);
    // #endif
    if (root.node().target==this) {
      trace(MENU_DEBUG_OUT<<"printMenu"<<endl);
      out.printMenu(root.node(), panelNr);
    } else {
      trace(MENU_DEBUG_OUT<<"previewMenu"<<endl);
      out.previewMenu(root,*(menuNode*)this,panelNr);
    }
    // #ifdef MENU_FMT_WRAPS
    //   out.fmtEnd(*this,menuOut::fmtBody,root.node(),idx);
    // #endif
  }
  return r;
}

#ifdef MENU_ASYNC
bool prompt::async(const char*uri,navRoot& root,idx_t lvl) {
  trace(MENU_DEBUG_OUT<<"prompt::async ["<<uri<<"]"<<endl;);
  return true;
}
idx_t menuNode::parseUriNode(const char*&uri) {
  if (*uri=='/') uri++;//TODO check who does this part!
  bool neg=false;
  if (*uri=='-') {
    uri++;
    neg=true;
  }
  assert(*uri=='-'||strchr(numericChars,uri[0]));
  int n=0;
  while (*uri) {
    char* d=strchr(numericChars,uri[0]);
    if (d) n=n*10+((*d)-'0');
    else break;
    uri++;
  }
  //state transformation (for events preservation)
  if (neg) n=sz()-n;//allow negative index from end of list
  if (n<0) n=0;
  else if (n>=sz()) n=sz()-1;//never trusting web!
  return n;
}

bool menuNode::async(const char*uri,navRoot& root,idx_t lvl) {
  trace(MENU_DEBUG_OUT<<*(prompt*)this<<" menuNode::async "<<uri<<" lvl:"<<lvl<<" root.level:"<<root.level<<endl);
  // assert(root.path[lvl].target==this);
  if ((!uri[0])||(uri[0]=='/'&&!uri[1])) {
    root.escTo(lvl);
    trace(MENU_DEBUG_OUT<<*(prompt*)this<<" async true! "<<uri<<endl);
    return true;
  }
  if (uri[0]=='/') uri++;//TODO check who does this part!
  assert(strchr(numericChars,uri[0]));
  int n=parseUriNode(uri);
  trace(MENU_DEBUG_OUT<<"n:"<<n<<" sel:"<<root.path[lvl].sel<<endl);
  if (!(root.path[lvl].sel==n&&root.path[lvl+1].target==&operator[](n)&&root.level>lvl)) {
    root.escTo(lvl/*+(lvl&&root.path[lvl].sel==n?-1:0)*/);
    root.doNav(navCmd(idxCmd,n));
  }
  return operator[](n).async(uri,root,lvl+1);
}
#endif

void textField::doNav(navNode& nav,navCmd cmd) {
  trace(MENU_DEBUG_OUT<<"textField::doNav:"<<cmd.cmd<<endl);
  switch(cmd.cmd) {
    case enterCmd:
      if (edited&&!charEdit) {
        edited=false;
        cursor=0;
        nav.root->exit();
      } else {
        charEdit=!charEdit;
        dirty=true;
        edited=true;
      }
      break;
    case escCmd:
      dirty=true;
      if (charEdit) charEdit=false;
      else {
        edited=false;
        cursor=0;
        nav.root->exit();
      }
      break;
    case upCmd:
      if (charEdit) {
        const char* v=validator(cursor);
        char *at=strchr(v,buffer()[cursor]);
        idx_t pos=at?at-v+1:1;
        if (pos>=(idx_t)strlen(v)) pos=0;
        buffer()[cursor]=v[pos];
        dirty=true;
      } else {
        if(cursor<(idx_t)strlen(buffer())-1) cursor++;
        edited=false;
      }
      dirty=true;
      break;
    case downCmd:
      if (charEdit) {
        const char* v=validator(cursor);
        char *at=strchr(v,buffer()[cursor]);//at is not stored on the class to save memory
        idx_t pos=at?at-v-1:0;
        if (pos<0) pos=strlen(v)-1;
        buffer()[cursor]=v[pos];
        dirty=true;
      } else {
        if (cursor) cursor--;
        edited=false;
      }
      dirty=true;
      break;
    default:break;
  }
  trace(MENU_DEBUG_OUT<<"cursor:"<<cursor<<endl);
}

Used textField::printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t panelNr) {
  trace(MENU_DEBUG_OUT<<*this<<" textField::printTo"<<endl);
  // out.fmtStart(menuOut::fmtPrompt,root.node(),idx);
  idx_t at=0;
  bool editing=this==root.navFocus;
  trace(MENU_DEBUG_OUT<<"editing:"<<editing<<" len:"<<len;)
  idx_t l=navTarget::printTo(root,sel,out,idx,len,panelNr);
  if (l<len) {
    out.write(editing?":":" ");
    l++;
  }
  // idx_t c=l;
  //idx_t top=out.tops[root.level];
  #ifdef MENU_FMT_WRAPS
    out.fmtStart(*this,menuOut::fmtTextField,root.node(),idx);
  #endif
  idx_t tit=hasTitle(root.node())?1:0;
  idx_t line=idx+tit;//-out.tops[root.level];
  trace(MENU_DEBUG_OUT<<" tit:"<<tit<<" line:"<<line<<" cursor:"<<cursor<<" l:"<<l<<" len:"<<len<<endl;)
  idx_t c=l+1;
  while(buffer()[at]&&l++<len)
    if (at==cursor&&editing) {
      // MENU_DEBUG_OUT<<"idx:"<<idx<<" line:"<<line<<" at:"<<at<<" l:"<<l<<endl;
      // c=l+1;
      l+=out.startCursor(root,l,line,charEdit);//draw textual cursor or color code start
      out.write(buffer()[at++]);//draw focused character
      l+=out.endCursor(root,l,line,charEdit);//draw textual cursor or color code end
    } else out.write(buffer()[at++]);
  out.editCursor(root,c+cursor,line,editing,charEdit);//reposition a non text cursor
  #ifdef MENU_FMT_WRAPS
    out.fmtEnd(*this,menuOut::fmtTextField,root.node(),idx);
  #endif
  return l;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// menuNode - any menu object that has a list of members
//
////////////////////////////////////////////////////////////////////////////////
bool menuNode::changed(const navNode &nav,const menuOut& out,bool sub,bool test) {
  trace(MENU_DEBUG_OUT<<*this<<" menuNode::changed"<<endl);
  return menuNode::_changes<false>(nav,out,sub,test);
  /*if (dirty) {
    trace(if (test) MENU_DEBUG_OUT<<"just dirty!"<<endl);
    return true;
  }
  bool appd=has((systemStyles)(_asPad|_parentDraw));
  if (appd) {
    trace(MENU_DEBUG_OUT<<"appd!"<<endl;);
    for(int i=0;i<sz();i++)
      if (operator[](i).changed(nav,out,false,test)) {
        // trace(if (test) MENU_DEBUG_OUT<<"APPD! "<<operator[](i).type()<<endl);
        return true;
      }
  } else {
    trace(MENU_DEBUG_OUT<<*this<<"!appd"<<endl;);
    if (!(nav.target==this&&sub)) {
      trace(if (test&&dirty) MENU_DEBUG_OUT<<"indirect!"<<endl);
      return dirty;// second hand check, just report self
    }
    idx_t level=nav.root->level;
    if (parentDraw()) {
      trace(MENU_DEBUG_OUT<<"return changed of parent-draw element"<<endl);
      trace(if (test) MENU_DEBUG_OUT<<"parentDraw()!"<<endl);
      return nav.root->path[level-1].target->changed(nav.root->path[level-1],out,sub,test);
    }
    // idx_t tit=hasTitle(nav.root->path[lev])?1:0;//TODO: this might not be correct.. checking
    idx_t my=out.maxY()-((has(showTitle)||(nav.root->showTitle&&!has(noTitle)))?1:0);
    // trace(MENU_DEBUG_OUT<<"level:"<<level<<" target:"<<*nav.root->navFocus<<" "<<nav.root->navFocus->has(_parentDraw)<<" "<<nav.root->navFocus->has(_asPad)<<endl);
    // idx_t lev=level-(nav.root->navFocus->has(_parentDraw)&&nav.root->navFocus->isMenu());
    idx_t t=out.tops[level-nav.root->navFocus->has(_parentDraw)&&has(_asPad)];
    // trace(MENU_DEBUG_OUT<<"tit:"<<tit<<endl;);
    // idx_t t=out.tops[lev];
    trace(MENU_DEBUG_OUT<<"t:"<<t<<endl;);
    if (sub) for(int i=0;i<my;i++,t++) {
      if (t>=sz()) break;
      trace(MENU_DEBUG_OUT<<"checking:"<<operator[](t)<<endl);
      if (operator[](t).changed(nav,out,false,test)) {
        trace(if (test) MENU_DEBUG_OUT<<"sub changed!"<<endl);
        return true;
      }
    }
  }
  return false;*/
}

void menuNode::clearChanged(const navNode &nav,const menuOut& out,bool sub) {
  trace(MENU_DEBUG_OUT<<" menuOut::clearChanged "<<nav);
  _changes<true>(nav,out,sub,false);
  /*dirty=false;
  if (has((systemStyles)(_asPad|_parentDraw))) {
    for(int i=0;i<sz();i++)
      operator[](i).clearChanged(nav,out,false);
  } else {
    if (!(nav.target==this&&sub)) return;
    idx_t level=nav.root->level;
    if (parentDraw())
      return nav.root->path[level-1].target->clearChanged(nav.root->path[level-1],out,sub);
    idx_t my=out.maxY()-((has(showTitle)||(nav.root->showTitle&&!has(noTitle)))?1:0);
    // idx_t lev=level-(nav.root->navFocus->has(_parentDraw)&&nav.root->navFocus->isMenu());
    // idx_t t=out.tops[lev];
    idx_t t=out.tops[level-nav.root->navFocus->has(_parentDraw)&&has(_asPad)];
    for(idx_t i=0;i<my;i++,t++) {//only signal visible
      if (t>=sz()) break;//menu ended
      operator[](t).clearChanged(nav,out,false);
    }
  }
  #ifdef MENU_DEBUG
  if(changed(nav,out,sub,true)) {
    MENU_DEBUG_OUT<<"ERROR clear changed fail!"<<endl;
    MENU_DEBUG_OUT<<*this<<endl;
    MENU_DEBUG_OUT<<"level:"<<nav.root->level<<endl;
    // MENU_DEBUG_OUT<<"type:"<<type()<<endl;
    MENU_DEBUG_OUT.flush();
    while(1);
  }
  #endif*/
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// navTarget - any menu object that can process keys/input
//
////////////////////////////////////////////////////////////////////////////////

void navTarget::doNav(navNode& nav,navCmd cmd) {
  trace(MENU_DEBUG_OUT<<"navTarget::doNav"<<endl);
  nav.doNavigation(cmd);
}

void navTarget::parseInput(navNode& nav,menuIn& in) {
  trace(MENU_DEBUG_OUT<<"navTarget::parseInput"<<endl);
  doNav(nav,nav.navKeys(in.read()));
}

void textField::parseInput(navNode& nav,menuIn& in) {
  trace(MENU_DEBUG_OUT<<"navTarget::parseInput"<<endl);
  if (/*charEdit&&*/in.available()) {
    char c=in.peek();
    if (options->useNavChars&&(c==options->getCmdChar(upCmd)
      ||c==options->getCmdChar(downCmd)
      ||c==options->getCmdChar(enterCmd)
      ||c==options->getCmdChar(escCmd))) {
        navTarget::parseInput(nav,in);
        return;
      }
    switch(c) {//special cases
      case 0x0D://enter
        in.read();
        charEdit=false;
        dirty=true;
        // edited=false;
        cursor=0;
        nav.root->exit();
        return;
      case 0x08://backspace
        in.read();
        buffer()[cursor]=validator(cursor)[0];
        if (cursor) cursor--;
        dirty=true;
        return;
      default: {
        const char* v=validator(cursor);
        char *at=strchr(v,c);
        if (at) {
          in.read();
          buffer()[cursor]=c;
          if (cursor<(idx_t)strlen(buffer())-1) cursor++;
          dirty=true;
          return;
        }
        // MENU_DEBUG_OUT<<hex(c)<<endl;
        navTarget::parseInput(nav,in);
      }
    }
  }
}

#ifdef MENU_ASYNC
bool textField::async(const char*uri,navRoot& root,idx_t lvl=0) {
  trace(MENU_DEBUG_OUT<<"textField::async "<<uri<<endl);
  if ((!*uri)||(uri[0]=='/'&&!uri[1])) return true;
  if (uri[0]=='/') {
    StringStream i(++uri);
    while(i.available()) parseInput(root.node(), i);
    trace(MENU_DEBUG_OUT<<"textField::enterCmd"<<endl);
    doNav(root.node(),escCmd);
    return true;
  }
  return true;
}

bool fieldBase::async(const char *uri,navRoot& root,idx_t lvl) {
  trace(MENU_DEBUG_OUT<<"fieldBase::async "<<uri<<endl);
  if ((!*uri)||(uri[0]=='/'&&!uri[1])) return true;
  else if (uri[0]=='/') {
    StringStream i(++uri);
    parseInput(root.node(), i);
    return true;
  }
  return true;
}
#endif

void fieldBase::doNav(navNode& nav,navCmd cmd) {
  trace(MENU_DEBUG_OUT<<"fieldBase::doNav "<<cmd;MENU_DEBUG_OUT<<endl);
  switch(cmd.cmd) {
    //by default esc and enter cmds do the same by changing the value
    //it might be set by numeric parsing when allowed
    case idxCmd: //MENU_DEBUG_OUT<<"menuField::doNav with idxCmd"<<endl;
    case escCmd:
      tunning=true;//prepare for exit
    case enterCmd:
      if (tunning||nav.root->nav2D||!canTune()) {//then exit edition
        tunning=false;
        dirty=true;
        constrainField();
        nav.event(nav.root->useUpdateEvent?updateEvent:enterEvent);
        nav.root->exit();
        return;
      } else tunning=true;
      dirty=true;
      break;
    case upCmd:
      stepit(1);
      break;
    case downCmd:
      stepit(-1);
      break;
    default:break;
  }
  /*if (ch==options->getCmdChar(enterCmd)&&!tunning) {
    nav.event(enterEvent);
  }*/
  if (dirty)//sending enter or update event
    nav.event(nav.root->useUpdateEvent?updateEvent:enterEvent);
}

Used fieldBase::printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t panelNr) {
  trace(MENU_DEBUG_OUT<<"fieldBase::printTo"<<endl);
  idx_t l=prompt::printTo(root,sel,out,idx,len,panelNr);
  bool ed=this==root.navFocus;
  //bool sel=nav.sel==i;
  if (l<len) {
    //TODO: remove cursor when printing asPad menus
    // if(!root.node().has(_asPad))//do notdraw edit cursors for pad menu items?
    //this is not really working!
    out.print((root.navFocus==this&&sel)?(tunning?'>':':'):' ');
    l++;
    if (l<len) {
      #ifdef MENU_FMT_WRAPS
        out.fmtStart(*this,menuOut::fmtField,root.node(),idx);
      #endif
      out.setColor(valColor,sel,enabled,ed);
      //out<<reflex;
      l+=printReflex(out);//NOTE: this can exceed the limits!
      #ifdef MENU_FMT_WRAPS
        out.fmtEnd(*this,menuOut::fmtField,root.node(),idx);
      #endif
      if (l<len) {
        #ifdef MENU_FMT_WRAPS
          out.fmtStart(*this,menuOut::fmtUnit,root.node(),idx);
        #endif
        out.setColor(unitColor,sel,enabled,ed);
        l+=print_P(out,units(),len);
        #ifdef MENU_FMT_WRAPS
          out.fmtEnd(*this,menuOut::fmtUnit,root.node(),idx);
        #endif
      }
    }
  }
  return l;
}

/////////////////////////////////////////////////////////////////
Used menuVariantBase::printTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t panelNr) {
  trace(MENU_DEBUG_OUT<<"menuVariantBase::printTo"<<endl);
  idx_t l=len;
  l-=prompt::printTo(root,sel,out,idx,len,panelNr);
  idx_t at=sync(sync());
  bool ed=this==root.navFocus;
  //bool sel=nav.sel==i;
  if (len-l<0) return 0;
  out.print(this==&root.active()?':':' ');
  l--;
  #ifdef MENU_FMT_WRAPS
    if (out.fmtStart(*this,type()==selectClass?menuOut::fmtSelect:menuOut::fmtChoose,root.node(),idx)==proceed) {
  #endif
    // MENU_DEBUG_OUT<<"variant ";
    // print_P(MENU_DEBUG_OUT,operator[](at).getText());
    // MENU_DEBUG_OUT<<endl;
    out.setColor(valColor,sel,prompt::enabled,ed);
    if (l>0) l-=operator[](at).printRaw(out,l);
  #ifdef MENU_FMT_WRAPS
    }
  #endif
  #ifdef MENU_FMT_WRAPS
  out.fmtEnd(*this,type()==selectClass?menuOut::fmtSelect:menuOut::fmtChoose,root.node(),idx);
  #endif
  trace(MENU_DEBUG_OUT<<"menuVariantBase::printTo ended!"<<endl);
  return len-l;
}

idx_t menuVariantBase::togglePrintTo(navRoot &root,bool sel,menuOut& out, idx_t idx,idx_t len,idx_t panelNr) {
  trace(MENU_DEBUG_OUT<<"menuVariantBase::togglePrintTo"<<endl);
  idx_t l=prompt::printTo(root,sel,out,idx,len,panelNr);
  idx_t at=sync(sync());
  bool ed=this==root.navFocus;
  //bool sel=nav.sel==i;
  out.setColor(valColor,sel,prompt::enabled,ed);
  //out<<menuNode::operator[](at);
  if (len-l>0) {
    #ifdef MENU_FMT_WRAPS
  out.fmtStart(*this,menuOut::fmtToggle,root.node(),idx);
    #endif
    l+=operator[](at).printRaw(out,len-l);
    #ifdef MENU_FMT_WRAPS
  out.fmtEnd(*this,menuOut::fmtToggle,root.node(),idx);
    #endif
  }
  return l;
}

void menuVariantBase::doNav(navNode& nav,navCmd cmd) {
  trace(MENU_DEBUG_OUT<<"menuVariantBase::doNav"<<endl);
  nav.sel=sync();
  navCmd c=nav.doNavigation(cmd);
  sync(nav.sel);
  if (c.cmd==enterCmd) {
    nav.root->exit();
  }
}

template<bool clear>
bool menuNode::_changes(const navNode &nav,const menuOut& out,bool sub,bool test) {
  _trace(MENU_DEBUG_OUT<<*this<<" menuNode::"<<(clear?"clearChanged":"changed")<<" test:"<<test<<endl);
  if (clear) dirty=false;
  else if (dirty) {
    _trace(if (test) MENU_DEBUG_OUT<<"just dirty!"<<endl);
    return true;
  }
  if (has((systemStyles)(_asPad|_parentDraw))) {
    for(int i=0;i<sz();i++)
      if (clear) operator[](i).clearChanged(nav,out,false);
      else if (operator[](i).changed(nav,out,false,test)) {
        _trace(if (test) MENU_DEBUG_OUT<<"APPD! "<<operator[](i)<<endl);
        return true;
      }
  } else {
    // if (!(nav.target==this&&sub)) return;???
    if (!(clear||(nav.target==this&&sub))) {
      _trace(if (test&&dirty) MENU_DEBUG_OUT<<"indirect!"<<endl);
      return dirty;// second hand check, just report self
    }
    idx_t level=nav.root->level;
    if (parentDraw()) {
      if (clear) nav.root->path[level-1].target->clearChanged(nav.root->path[level-1],out,sub);
      else {
        _trace(MENU_DEBUG_OUT<<"return changed of parent-draw element"<<endl);
        _trace(if (test) MENU_DEBUG_OUT<<"parentDraw()!"<<endl);
        return nav.root->path[level-1].target->changed(nav.root->path[level-1],out,sub,test);
      }
    }
    // idx_t tit=hasTitle(nav.root->path[lev])?1:0;//TODO: this might not be correct.. checking
    idx_t my=out.maxY()-((has(showTitle)||(nav.root->showTitle&&!has(noTitle)))?1:0);
    // trace(MENU_DEBUG_OUT<<"level:"<<level<<" target:"<<*nav.root->navFocus<<" "<<nav.root->navFocus->has(_parentDraw)<<" "<<nav.root->navFocus->has(_asPad)<<endl);
    // idx_t lev=level-(nav.root->navFocus->has(_parentDraw)&&(nav.root->navFocus->isMenu()||nav.root->navFocus->has(_asPad)));
    // idx_t t=out.tops[lev];
    idx_t t=out.tops[level];//-nav.root->navFocus->has(_parentDraw)&&has(_asPad)];
    trace(MENU_DEBUG_OUT<<"t:"<<t<<endl;);
    if (sub) for(int i=0;i<my;i++,t++) {
      if (t>=sz()) break;
      if (clear) operator[](t).clearChanged(nav,out,false);
      {
        trace(MENU_DEBUG_OUT<<"checking:"<<operator[](t)<<endl);
        if (operator[](t).changed(nav,out,false,test)) {
          _trace(if (test) MENU_DEBUG_OUT<<"sub changed!"<<endl);
          return true;
        }
      }
    }
  }
  if (clear) {
    #ifdef MENU_DEBUG
      if(changed(nav,out,sub,true)) {
        MENU_DEBUG_OUT<<"\nERROR clear changed fail!"<<endl;
        MENU_DEBUG_OUT<<*this<<endl;
        MENU_DEBUG_OUT<<"level:"<<nav.root->level<<endl;
        // MENU_DEBUG_OUT<<"type:"<<type()<<endl;
        MENU_DEBUG_OUT.flush();
        // while(1);
        return false;
      }
    #endif
    return true;
  } else return false;
}
