/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Scott Moreau
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#pragma once

#include "wayfire-control-client-protocol.h"

#define REQUEST_MOVE       1 << 1
#define REQUEST_RESIZE     1 << 2
#define REQUEST_MAXIMIZE   1 << 3
#define REQUEST_UNMAXIMIZE 1 << 4
#define REQUEST_MINIMIZE   1 << 5
#define REQUEST_UNMINIMIZE 1 << 6
#define REQUEST_WS_SWITCH  1 << 7
#define REQUEST_FOCUS      1 << 8
#define REQUEST_CLOSE      1 << 9

class WfCtrl
{
  public:
    WfCtrl(int argc, char *argv[]);
    ~WfCtrl();

    wl_display *display;
    wf_ctrl_base *wf_control_manager;
    int running;
    void run();
};

void do_key(WfCtrl *, int argc, char *argv[]);
void do_button(WfCtrl *, int argc, char *argv[]);
void do_mousemove(WfCtrl *, int argc, char *argv[]);
