// SPDX-License-Identifier: MIT
// Author:  Giovanni Santini
// Mail:    giovanni.santini@proton.me
// Github:  @San7o
//
// micro-sprite
// ============
//
// Simple drawing tool
//

// Luigi: A barebones single-header GUI library for Win32, X11 and
// Essence. (https://github.com/nakst/luigi)
#define UI_LINUX
#define UI_IMPLEMENTATION
#include "luigi.h"

#include <stdio.h>

//
// Global UI elements
//

UIWindow *window;

UIButton *buttonClear;
UIButton *buttonLevel;
UIButton *buttonQuit;

UIColorPicker *colorPicker;
uint32_t color = UI_COLOR_FROM_FLOAT(1.0f, 1.0f, 1.0f);

UIImageDisplay *canvas;
unsigned int lastX = 0;
unsigned int lastY = 0;

#define CANVAS_WIDTH 1000
#define CANVAS_HEIGHT 1000
#define CANVAS_STRIDE CANVAS_WIDTH * (sizeof(uint32_t))
uint32_t layer0[CANVAS_HEIGHT * CANVAS_WIDTH] = {0};
uint32_t layer1[CANVAS_HEIGHT * CANVAS_WIDTH] = {0};
uint32_t layer2[CANVAS_HEIGHT * CANVAS_WIDTH] = {0};
uint32_t *currentLayer = layer0;

//
// UI Callbacks
//

int ButtonMessageClear(UIElement *element, UIMessage message, int di, void *dp)
{
  if (message == UI_MSG_CLICKED)
  {
    for (int i = 0; i < CANVAS_WIDTH; ++i)
      for (int j = 0; j < CANVAS_HEIGHT; ++j)
        currentLayer[i * CANVAS_HEIGHT + j] = UI_COLOR_FROM_FLOAT(0.0f, 0.0f, 0.0f);
    UIImageDisplaySetContent(canvas, (uint32_t*)currentLayer, CANVAS_WIDTH,
                             CANVAS_HEIGHT, CANVAS_STRIDE);
    UIElementRefresh(&canvas->e);
  }
  return 0;
}

int ColorPickerMessage(UIElement *element, UIMessage message, int di, void *dp)
{
  if (message == UI_MSG_VALUE_CHANGED)
  {
    UIColorToRGB(colorPicker->hue, colorPicker->saturation, colorPicker->value, &color);
  }
  return 0;
}

static char* layer_names[] = {
  "Layer 0",
  "Layer 1",
  "Layer 2",
};

void LevelCallback(void *cp /* selected layer */) {
  uint64_t layer = (uint64_t) cp;

  buttonLevel->label = layer_names[layer];
  if (layer == 0)
  {
    currentLayer = layer0;
  }
  if (layer == 1)
  {
    currentLayer = layer1;
  }
  else if (layer == 2)
  {
    currentLayer = layer2;
  }
  UIElementRefresh(&buttonLevel->e);
  UIImageDisplaySetContent(canvas, (uint32_t*)currentLayer, CANVAS_WIDTH,
                           CANVAS_HEIGHT, CANVAS_STRIDE);
  UIElementRefresh(&canvas->e);
}

int ButtonMessageLevel(UIElement *element, UIMessage message, int di, void *dp)
{
  if (message == UI_MSG_CLICKED)
  {
    UIMenu *menu = UIMenuCreate(element, 0);
    UIMenuAddItem(menu, 0, layer_names[0], -1, LevelCallback, (void*) (uint64_t) 0);
    UIMenuAddItem(menu, 0, layer_names[1], -1, LevelCallback, (void*) (uint64_t) 1);
    UIMenuAddItem(menu, 0, layer_names[2], -1, LevelCallback, (void*) (uint64_t) 2);
    UIMenuShow(menu);
  }
  return 0;
}

int ButtonMessageQuit(UIElement *element, UIMessage message, int di, void *dp)
{
  if (message == UI_MSG_CLICKED)
  {
    ui.quit = true;
  }
  return 0;
}

int CanvasMessage(UIElement *element, UIMessage message, int di, void *dp)
{
  if (message == UI_MSG_LEFT_UP)
  {
    lastX = 0;
    lastY = 0;
  }
  if (message == UI_MSG_MOUSE_DRAG)
  {
    unsigned int x = window->cursorX - element->bounds.l;
    unsigned int y = window->cursorY - element->bounds.t;
    if (x >= CANVAS_WIDTH) x = CANVAS_WIDTH - 1;
    if (y >= CANVAS_HEIGHT) y = CANVAS_HEIGHT - 1;
    //printf("Mouse moved! x: %d, y: %d\n", x, y);

    if (lastX == 0 || lastY == 0)
    {
      currentLayer[y * CANVAS_WIDTH + x] = color;
      lastX = x;
      lastY = y;
      return 0;
    }
    
    // interpolate from the last point
    unsigned int dx = lastX - x;
    unsigned int dy = lastY - y;
    float delta = dy / (float) dx;    
    if (delta < 1.0 && delta > -1.0)
    {
      if (x > lastX)
      {
        for (unsigned int i = lastX; i < x; ++i)
        {
          unsigned int y_2 = delta * (i - lastX) + lastY;
          if (y_2 >= CANVAS_HEIGHT) continue;
            
          currentLayer[y_2 * CANVAS_WIDTH + i] = color;
        }
      }
      else {
        for (unsigned int i = lastX; i >= x; --i)
        {
          unsigned int y_2 = delta * (i - x) + lastY;
          if (y_2 >= CANVAS_HEIGHT) continue;
        
          currentLayer[y_2 * CANVAS_WIDTH + i] = color;
        }
      }
    }
    else
    {
      if (y > lastY)
      {
        for (unsigned int i = lastY; i < y; ++i)
        {
          unsigned int x_2 = (dx / (float) dy) * (i - lastY) + lastX;
          if (x_2 >= CANVAS_WIDTH) continue;
            
          currentLayer[i * CANVAS_WIDTH + x_2] = color;
        }
      }
      else {
        for (unsigned int i = lastY; i >= y; --i)
        {
          unsigned int x_2 = (dx / (float) dy) * (i - y) + lastX;
          if (x_2 >= CANVAS_WIDTH) continue;
        
          currentLayer[i * CANVAS_WIDTH + x_2] = color;
        }
      }
    }
    lastX = x;
    lastY = y;

    UIImageDisplaySetContent(canvas, (uint32_t*)currentLayer, CANVAS_WIDTH,
                             CANVAS_HEIGHT, CANVAS_STRIDE);
    UIElementRefresh(&canvas->e);
  }
  return 0;
}

int main(void)
{
  UIInitialise();

  window = UIWindowCreate(0, 0, "micro-sprite", 500, 500);
  if (!window)
  {
    fprintf(stderr, "Error creating window\n");
    return 1;
  }
  
  UISplitPane *split = UISplitPaneCreate(&window->e,
                                         0 /* | UI_SPLIT_PANE_VERTICAL */, 0.3f);

  // Left split
  {
    UIPanel *panel = UIPanelCreate(&split->e, UI_PANEL_GRAY | UI_PANEL_MEDIUM_SPACING);
    
    colorPicker = UIColorPickerCreate(&panel->e, 0);
    colorPicker->saturation = 0;
    colorPicker->value = 1;
    colorPicker->e.messageUser = ColorPickerMessage;

    buttonLevel = UIButtonCreate(&panel->e, 0, "Layer 0", -1);
    buttonLevel->e.messageUser = ButtonMessageLevel;

    buttonClear = UIButtonCreate(&panel->e, 0, "Clear", -1);
    buttonClear->e.messageUser = ButtonMessageClear;
    
    buttonQuit = UIButtonCreate(&panel->e, 0, "Quit", -1);
    buttonQuit->e.messageUser = ButtonMessageQuit;
  }

  // Right split
  {
    UIPanel *panel = UIPanelCreate(&split->e, UI_PANEL_GRAY);
    canvas = UIImageDisplayCreate(&panel->e, _UI_IMAGE_DISPLAY_ZOOM_FIT,
                                  (uint32_t*)currentLayer,
                                  CANVAS_WIDTH, CANVAS_HEIGHT, CANVAS_STRIDE);
    canvas->e.messageUser = CanvasMessage;
  }

	return UIMessageLoop();
}
