#include "SSD1306Ui.h"


SSD1306Ui::SSD1306Ui(SSD1306 *display) {
  this->display = display;
}

void SSD1306Ui::init() {
  this->display->init();
}

void SSD1306Ui::setTargetFPS(byte fps){
  int oldInterval = this->updateInterval;
  this->updateInterval = ((float) 1.0 / (float) fps) * 1000;

  // Calculate new ticksPerFrame
  float changeRatio = oldInterval / this->updateInterval;
  this->ticksPerFrame *= changeRatio;
  this->ticksPerTransition *= changeRatio;
}

// -/------ Automatic controll ------\-

void SSD1306Ui::enableAutoTransition(){
  autoTransition = true;
}
void SSD1306Ui::disableAutoTransition(){
  autoTransition = false;
}
void SSD1306Ui::setAutoTransitionForwards(){
  this->frameTransitionDirection = 1;
}
void SSD1306Ui::setAutoTransitionBackwards(){
  this->frameTransitionDirection = 1;
}
void SSD1306Ui::setTimePerFrame(int time){
  this->ticksPerFrame = (int) ( (float) time / (float) updateInterval);
}
void SSD1306Ui::setTimePerTransition(int time){
  this->ticksPerTransition = (int) ( (float) time / (float) updateInterval);
}


// -/------ Customize indicator position and style -------\-
void SSD1306Ui::setIndicatorPosition(IndicatorPosition pos) {
  this->indicatorPosition = pos;
  this->dirty = true;
}
void SSD1306Ui::setIndicatorDirection(IndicatorDirection dir) {
  this->indicatorDirection = dir;
}
void SSD1306Ui::setActiveSymbole(const char* symbole) {
  this->activeSymbole = symbole;
  this->dirty = true;
}
void SSD1306Ui::setInactiveSymbole(const char* symbole) {
  this->inactiveSymbole = symbole; 
  this->dirty = true;
}


// -/----- Frame settings -----\-
void SSD1306Ui::setFrameAnimation(AnimationDirection dir) {  
  this->frameAnimationDirection = dir;
}
void SSD1306Ui::setFrames(bool (*frameFunctions[])(SSD1306 *display, int x, int y), int frameCount) {
  this->frameCount     = frameCount;
  this->frameFunctions = frameFunctions;
}

// -/----- Overlays ------\-
void SSD1306Ui::setOverlays(bool (*overlayFunctions[])(SSD1306 *display), int overlayCount){
  this->overlayCount     = overlayCount;
  this->overlayFunctions = overlayFunctions;
}


// -/----- Manuel control -----\-
void SSD1306Ui::nextFrame() {
  this->frameState = IN_TRANSITION;
  this->ticksSinceLastStateSwitch = 0;
  this->frameTransitionDirection = 1;
}
void SSD1306Ui::previousFrame() {
  this->frameState = IN_TRANSITION;
  this->ticksSinceLastStateSwitch = 0;
  this->frameTransitionDirection = -1;
}


// -/----- State information -----\-
FrameState SSD1306Ui::getFrameState(){
  return this->frameState;
}
int SSD1306Ui::getCurrentFrame(){
  return this->currentFrame;
}


int SSD1306Ui::update(){
  int timeBudget = this->updateInterval - (millis() - this->lastUpdate);
  if ( timeBudget <= 0) {

    // Implement frame skipping to ensure time budget is keept
    if (this->autoTransition && this->lastUpdate != 0) this->ticksSinceLastStateSwitch += abs(timeBudget) / this->updateInterval;
    
    this->lastUpdate = millis();
    this->tick();
  } 
  return timeBudget;
}


void SSD1306Ui::tick() {
  this->ticksSinceLastStateSwitch++;

  switch (this->frameState) {
    case IN_TRANSITION:
        this->dirty = true;
        if (this->ticksSinceLastStateSwitch >= this->ticksPerTransition){
          this->frameState = FIXED;
          this->currentFrame = getNextFrameNumber();
          this->ticksSinceLastStateSwitch = 0;
        }
      break;
    case FIXED:
      if (this->ticksSinceLastStateSwitch >= this->ticksPerFrame){
          if (this->autoTransition){
            this->frameState = IN_TRANSITION;
            this->dirty = true;
          }
          this->ticksSinceLastStateSwitch = 0;
      }
      break;
  }
  
  if (this->dirty) {
    this->dirty = false;
    this->display->clear();
    this->drawIndicator();  
    this->drawFrame();
    this->drawOverlays();
    this->display->display();
  }
}

void SSD1306Ui::drawFrame(){
  switch (this->frameState){
     case IN_TRANSITION: {
       float progress = (float) this->ticksSinceLastStateSwitch / (float) this->ticksPerTransition;
       int x, y, x1, y1;
       switch(this->frameAnimationDirection){
        case SLIDE_LEFT:
          x = -128 * progress;
          y = 0;
          x1 = x + 128;
          y1 = 0;
          break;
        case SLIDE_RIGHT:
          x = 128 * progress;
          y = 0;
          x1 = x - 128;
          y1 = 0;
          break;
        case SLIDE_UP:
          x = 0;
          y = -64 * progress;
          x1 = 0;
          y1 = y + 64;
          break;
        case SLIDE_DOWN:
          x = 0;
          y = 64 * progress;
          x1 = 0;
          y1 = y - 64;
          break;
       }

       // Invert animation if direction is reversed.
       int dir = frameTransitionDirection >= 0 ? 1 : -1;
       x *= dir; y *= dir; x1 *= dir; y1 *= dir;

       this->dirty |= (*this->frameFunctions[this->currentFrame])(this->display, x, y);
       this->dirty |= (*this->frameFunctions[this->getNextFrameNumber()])(this->display, x1, y1);
       break;
     }
     case FIXED:
      this->dirty |= (*this->frameFunctions[this->currentFrame])(this->display, 0, 0);
      break;
  }
}

void SSD1306Ui::drawIndicator() {
    byte posOfCurrentFrame; 
    
    switch (this->indicatorDirection){
      case LEFT_RIGHT:
        posOfCurrentFrame = this->currentFrame;
        break;
      case RIGHT_LEFT:
        posOfCurrentFrame = (this->frameCount - 1) - this->currentFrame;
        break;
    }
    
    for (byte i = 0; i < this->frameCount; i++) {
      
      const char *xbm;
      
      if (posOfCurrentFrame == i) {
         xbm = this->activeSymbole;
      } else {
         xbm = this->inactiveSymbole;  
      }

      int x,y;
      switch (this->indicatorPosition){
        case TOP:
          y = 0;
          x = 64 - (12 * frameCount / 2) + 12 * i;
          break;
        case BOTTOM:
          y = 56;
          x = 64 - (12 * frameCount / 2) + 12 * i;
          break;
        case RIGHT:
          x = 120;
          y = 32 - (12 * frameCount / 2) + 12 * i;
          break;
        case LEFT:
          x = 0;
          y = 32 - (12 * frameCount / 2) + 12 * i;
          break;
      }
      
      this->display->drawXbm(x, y, 8, 8, xbm);
    }  
}

void SSD1306Ui::drawOverlays() {
 for (int i=0;i<this->overlayCount;i++){
    this->dirty |= (*this->overlayFunctions[i])(this->display);
 }
}

int SSD1306Ui::getNextFrameNumber(){
  int nextFrame = (this->currentFrame + this->frameTransitionDirection) % this->frameCount;
  if (nextFrame < 0){
    nextFrame = this->frameCount + nextFrame;
  }
  return nextFrame;  
}



