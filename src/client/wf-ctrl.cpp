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


#include <iostream>
#include <string.h>
#include <getopt.h>
#include <vector>

#include "wf-ctrl.hpp"

static void registry_add(void *data, struct wl_registry *registry,
    uint32_t id, const char *interface,
    uint32_t version)
{
    WfCtrl *wfm = (WfCtrl *) data;

    if (strcmp(interface, wf_ctrl_base_interface.name) == 0)
    {
        wfm->wf_control_manager = (wf_ctrl_base *)
            wl_registry_bind(registry, id,
            &wf_ctrl_base_interface, 1);
    }
}

static void registry_remove(void *data, struct wl_registry *registry,
    uint32_t id)
{
    WfCtrl *wfm = (WfCtrl *) data;

    wfm->running = 0;
}

static const struct wl_registry_listener registry_listener = {
    .global = registry_add,
    .global_remove = registry_remove,
};

static void receive_ack(void *data,
    struct wf_ctrl_base *wf_ctrl_base)
{
    WfCtrl *wfm = (WfCtrl *) data;

    wfm->running = 0;
}


static struct wf_ctrl_base_listener control_base_listener {
	.ack = receive_ack,
};

WfCtrl::WfCtrl(int argc, char *argv[])
{
    display = wl_display_connect(NULL);
    if (!display)
    {
        return;
    }

    wl_registry *registry = wl_display_get_registry(display);
    if (!registry)
    {
        return;
    }

    wl_registry_add_listener(registry, &registry_listener, this);

    wf_control_manager = NULL;
    wl_display_roundtrip(display);
    wl_registry_destroy(registry);
    if (!wf_control_manager)
    {
        std::cout << "Wayfire control protocol not advertised by compositor. Is wf-ctrl plugin enabled?" << std::endl;
        return;
    }

    wf_ctrl_base_add_listener(wf_control_manager,
        &control_base_listener, this);

    std::vector<int> view_ids;
    int request_mask = 0;
    int x, y, w, h, ws_x, ws_y;
    char *direction = NULL;

    struct option opts[] = {
        { "view-id",     required_argument, NULL, 'i' },
        { "move",        required_argument, NULL, 'm' },
        { "resize",      required_argument, NULL, 'r' },
        { "maximize",    no_argument,       NULL, 'X' },
        { "unmaximize",  no_argument,       NULL, 'x' },
        { "minimize",    no_argument,       NULL, 'N' },
        { "unminimize",  no_argument,       NULL, 'n' },
        { "switch-ws",   required_argument, NULL, 'w' },
        { 0,             0,                 NULL,  0  }
    };

    int c, i;
    while((c = getopt_long(argc, argv, "i:m:r:XxNnw:", opts, &i)) != -1)
    {
        switch(c)
        {
            case 'i':
                view_ids.push_back(atoi(optarg));
                break;

            case 'm':
                sscanf(optarg, "%d,%d", &x, &y);
                request_mask |= REQUEST_MOVE;
                break;

            case 'r':
                sscanf(optarg, "%dx%d", &w, &h);
                request_mask |= REQUEST_RESIZE;
                break;

            case 'X':
                request_mask |= REQUEST_MAXIMIZE;
                break;

            case 'x':
                request_mask |= REQUEST_UNMAXIMIZE;
                break;

            case 'N':
                request_mask |= REQUEST_MINIMIZE;
                break;

            case 'n':
                request_mask |= REQUEST_UNMINIMIZE;
                break;

            case 'w':
                if (sscanf(optarg, "%d,%d", &ws_x, &ws_y) != 2)
                {
                    direction = optarg;
                }
                request_mask |= REQUEST_WS_SWITCH;
                break;

            default:
                printf("Unsupported command line argument %s\n", optarg);
        }
    }

    for (auto view_id : view_ids)
    {
        if (request_mask & REQUEST_MOVE)
            wf_ctrl_base_move(wf_control_manager, view_id, x, y);
        if (request_mask & REQUEST_RESIZE)
            wf_ctrl_base_resize(wf_control_manager, view_id, w, h);
        if (request_mask & REQUEST_MAXIMIZE)
            wf_ctrl_base_maximize(wf_control_manager, view_id);
        if (request_mask & REQUEST_UNMAXIMIZE)
            wf_ctrl_base_unmaximize(wf_control_manager, view_id);
        if (request_mask & REQUEST_MINIMIZE)
            wf_ctrl_base_minimize(wf_control_manager, view_id);
        if (request_mask & REQUEST_UNMINIMIZE)
            wf_ctrl_base_unminimize(wf_control_manager, view_id);
    }

    if (request_mask & REQUEST_WS_SWITCH)
    {
        for (auto view_id : view_ids)
        {
            wf_ctrl_base_ws_switch_view_append(wf_control_manager, view_id);
        }
        if (direction)
        {
            wf_ctrl_base_ws_switch(wf_control_manager, direction);
        }
        else
        {
            wf_ctrl_base_ws_switch_abs(wf_control_manager, ws_x, ws_y);
        }
    }

    running = 1;
    while(running)
        wl_display_dispatch(display);

    wl_display_flush(display);
    wl_display_disconnect(display);
}

WfCtrl::~WfCtrl()
{
}

int main(int argc, char *argv[])
{
    WfCtrl(argc, argv);

    return 0;
}
