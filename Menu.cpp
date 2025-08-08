#include "Menu.h"
#include "Debug.h"

void Menu::begin(bool mode24) {
  _mode24 = mode24;
  _state = SHOW_TIME;
}

void Menu::enterMenu() { _state=MENU; _menuIdx=1; _disp.showMenuIndex(_menuIdx); DBG("[UI]->MENU\r\n"); }
void Menu::exitToTime(){ _state=SHOW_TIME; DBG("[UI] MENU->TIME\r\n"); }
void Menu::enterSetTime(){
  auto t=_rtc.now(); _eh=t.h; _em=t.m; _es=t.s; _field=0; _state=SET_TIME; DBG("[UI]->SET_TIME\r\n");
}
void Menu::saveTimeAndBackToMenu(){ _rtc.set({ _eh,_em,_es }); _state=MENU; _disp.showMenuIndex(_menuIdx); DBG("[UI] saved time\r\n"); }

void Menu::update(Encoder& enc) {
  enc.poll();

  // Press to enter menu while showing time
  if (_state==SHOW_TIME && enc.pressedEdge()) { enterMenu(); return; }

  // Encoder steps
  int8_t st = enc.step();
  if (st != 0) {
    if (_state==MENU) {
      if (st>0 && _menuIdx<2) _menuIdx++;
      if (st<0 && _menuIdx>1) _menuIdx--;
      _disp.showMenuIndex(_menuIdx);
    } else if (_state==SET_TIME) {
      if (_field==0){ int v=_eh + (st>0?1:-1); if(v<0)v=23; if(v>23)v=0; _eh=(uint8_t)v; }
      else if (_field==1){ int v=_em + (st>0?1:-1); if(v<0)v=59; if(v>59)v=0; _em=(uint8_t)v; }
      else { int v=_es + (st>0?1:-1); if(v<0)v=59; if(v>59)v=0; _es=(uint8_t)v; }
    }
  }

  // Button edges
  if (enc.releasedEdge()) {
    // short/long press based on hold duration can be added if needed
    if (_state==MENU) {
      if (_menuIdx==1) { _mode24 = !_mode24; _disp.showMenuIndex(_menuIdx); DBG("[UI] mode=%s\r\n", _mode24?"24h":"12h"); }
      else if (_menuIdx==2) { enterSetTime(); }
    } else if (_state==SET_TIME) {
      if (_field < 2) { _field++; }
      else { saveTimeAndBackToMenu(); }
    }
  }

  // Optional: long-press to exit menu (press+hold; not included to keep simple)
}

void Menu::render() {
  if (_state==SHOW_TIME) {
    uint32_t now=millis();
    if (now - _lastPush > PUSH_MS) {
      _lastPush = now;
      auto t = _rtc.now();
      _disp.showTimeDigits(t.h, t.m, t.s, _mode24);
    }
  } else if (_state==MENU) {
    _disp.showMenuIndex(_menuIdx);
  } else if (_state==SET_TIME) {
    _disp.showEditTimeBlink(_eh,_em,_es,_field);
  }
}
