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


#include <sys/time.h>
#include <wayfire/core.hpp>
#include <wayfire/view.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/output.hpp>
#include <wayfire/output-layout.hpp>
#include <wayfire/workspace-manager.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/nonstd/wlroots-full.hpp>
#include <linux/input-event-codes.h>
#include <wayfire/util/log.hpp>

#include "wayfire-control.hpp"
#include "wayfire-control-server-protocol.h"

extern "C"
{
#include <wlr/types/wlr_seat.h>
#include <wlr/backend/wayland.h>
#include <wlr/backend/multi.h>
#include <wlr/backend/headless.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/interfaces/wlr_keyboard.h>
#include <wlr/types/wlr_output_layout.h>
#include <libevdev/libevdev.h>
}

static void bind_manager(wl_client *client, void *data,
    uint32_t version, uint32_t id);

wayfire_control::wayfire_control()
{
    manager = wl_global_create(wf::get_core().display,
        &wf_ctrl_base_interface, 1, this, bind_manager);

    if (!manager)
    {
        LOGE("Failed to create wayfire_control interface");
        return;
    }

    auto& core = wf::get_core();
    backend = wlr_headless_backend_create(core.display);
    wlr_multi_backend_add(core.backend, backend);

    pointer  = wlr_headless_add_input_device(backend, WLR_INPUT_DEVICE_POINTER);
    keyboard = wlr_headless_add_input_device(backend, WLR_INPUT_DEVICE_KEYBOARD);

    if (core.get_current_state() == wf::compositor_state_t::RUNNING)
    {
        wlr_backend_start(backend);
    }
}

wayfire_control::~wayfire_control()
{
    auto& core = wf::get_core();
    wlr_multi_backend_remove(core.backend, backend);
    wlr_backend_destroy(backend);

    wl_global_destroy(manager);
}

wayfire_view view_from_id(int32_t id)
{
    if (id == -1)
    {
        return wf::get_core().get_active_output()->get_active_view();
    }

    for (auto& view : wf::get_core().get_all_views())
    {
        if (int32_t(view->get_id()) == id)
        {
            return view;
        }
    }

    return nullptr;
}

static void maximize(struct wl_client *client, struct wl_resource *resource, int view_id)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    view->tile_request(wf::TILED_EDGES_ALL);
    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void unmaximize(struct wl_client *client, struct wl_resource *resource, int view_id)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    view->tile_request(0);
    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void minimize(struct wl_client *client, struct wl_resource *resource, int view_id)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    view->minimize_request(true);
    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void unminimize(struct wl_client *client, struct wl_resource *resource, int view_id)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    view->minimize_request(false);
    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void focus(struct wl_client *client, struct wl_resource *resource, int view_id)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    auto output = view->get_output();

    if (!output)
    {
        return;
    }

    output->focus_view(view, true);
    output->workspace->request_workspace(
        output->workspace->get_view_main_workspace(view));
    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void close(struct wl_client *client, struct wl_resource *resource, int view_id)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    view->close();
    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void move(struct wl_client *client, struct wl_resource *resource, int view_id, int x, int y)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    view->move(x, y);
    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void resize(struct wl_client *client, struct wl_resource *resource, int view_id, int w, int h)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    view->resize(w, h);
    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void ws_switch_view_append(struct wl_client *client, struct wl_resource *resource, int view_id)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wayfire_view view = view_from_id(view_id);

    if (!view)
    {
        return;
    }

    wd->fixed_views.push_back(view);
}

static void ws_switch(struct wl_client *client, struct wl_resource *resource, const char *direction)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    auto output = wf::get_core().get_active_output();

    if (!output)
    {
        return;
    }

    auto ws = output->workspace->get_current_workspace();

    wayfire_view view;

    if (!strcmp(direction, "up"))
    {
        if (wd->fixed_views.empty())
        {
            output->workspace->request_workspace({ws.x, ws.y - 1});
        }
        else
        {
            output->workspace->request_workspace({ws.x, ws.y - 1}, wd->fixed_views);
        }
    }
    else if (!strcmp(direction, "down"))
    {
        if (wd->fixed_views.empty())
        {
            output->workspace->request_workspace({ws.x, ws.y + 1});
        }
        else
        {
            output->workspace->request_workspace({ws.x, ws.y + 1}, wd->fixed_views);
        }
    }
    else if (!strcmp(direction, "left"))
    {
        if (wd->fixed_views.empty())
        {
            output->workspace->request_workspace({ws.x - 1, ws.y});
        }
        else
        {
            output->workspace->request_workspace({ws.x - 1, ws.y}, wd->fixed_views);
        }
    }
    else if (!strcmp(direction, "right"))
    {
        if (wd->fixed_views.empty())
        {
            output->workspace->request_workspace({ws.x + 1, ws.y});
        }
        else
        {
            output->workspace->request_workspace({ws.x + 1, ws.y}, wd->fixed_views);
        }
    }

    wd->fixed_views.clear();

    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void ws_switch_abs(struct wl_client *client, struct wl_resource *resource, int x, int y)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    wf::point_t ws{x, y};

    auto output = wf::get_core().get_active_output();

    if (!output)
    {
        return;
    }

    wayfire_view view;

    if (wd->fixed_views.empty())
    {
        output->workspace->request_workspace(ws);
    }
    else
    {
        output->workspace->request_workspace(ws, wd->fixed_views);
    }

    wd->fixed_views.clear();

    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void keystroke(struct wl_client *client, struct wl_resource *resource, const char *key, int delay)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    int keycode = libevdev_event_code_from_name(EV_KEY, (std::string("KEY_") + key).c_str());
    wlr_event_keyboard_key ev;

    if (keycode == -1)
    {
        return;
    }

    ev.keycode = keycode;
    ev.state   = WL_KEYBOARD_KEY_STATE_PRESSED;
    ev.update_state = true;
    ev.time_msec    = wf::get_current_time();

    wlr_keyboard_notify_key(wd->keyboard->keyboard, &ev);

    wd->keyboard_stroke_delay.set_timeout(delay, [=] ()
    {
        wlr_event_keyboard_key ev;
        ev.keycode = keycode;
        ev.state   = WL_KEYBOARD_KEY_STATE_RELEASED;
        ev.update_state = true;
        ev.time_msec    = wf::get_current_time();

        wlr_keyboard_notify_key(wd->keyboard->keyboard, &ev);
        return false;
    });

    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void keydown(struct wl_client *client, struct wl_resource *resource, const char *key)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    int keycode = libevdev_event_code_from_name(EV_KEY, (std::string("KEY_") + key).c_str());
    wlr_event_keyboard_key ev;

    if (keycode == -1)
    {
        return;
    }

    ev.keycode = keycode;
    ev.state   = WL_KEYBOARD_KEY_STATE_PRESSED;
    ev.update_state = true;
    ev.time_msec    = wf::get_current_time();

    wlr_keyboard_notify_key(wd->keyboard->keyboard, &ev);

    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void keyup(struct wl_client *client, struct wl_resource *resource, const char *key)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    int keycode = libevdev_event_code_from_name(EV_KEY, (std::string("KEY_") + key).c_str());
    wlr_event_keyboard_key ev;

    if (keycode == -1)
    {
        return;
    }

    ev.keycode = keycode;
    ev.state   = WL_KEYBOARD_KEY_STATE_RELEASED;
    ev.update_state = true;
    ev.time_msec    = wf::get_current_time();

    wlr_keyboard_notify_key(wd->keyboard->keyboard, &ev);

    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void buttonstroke(struct wl_client *client, struct wl_resource *resource, const char *button, int delay)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    int buttoncode = libevdev_event_code_from_name(EV_KEY, (std::string("BTN_") + button).c_str());
    wlr_event_pointer_button ev;

    if (buttoncode == -1)
    {
        return;
    }
    ev.device    = wd->pointer;
    ev.button    = buttoncode;
    ev.state     = WLR_BUTTON_PRESSED;
    ev.time_msec = wf::get_current_time();
    wl_signal_emit(&wd->pointer->pointer->events.button, &ev);
    wl_signal_emit(&wd->pointer->pointer->events.frame, NULL);

    wd->button_stroke_delay.set_timeout(delay, [=] ()
    {
        wlr_event_pointer_button ev;
        ev.device    = wd->pointer;
        ev.button    = buttoncode;
        ev.state     = WLR_BUTTON_RELEASED;
        ev.time_msec = wf::get_current_time();
        wl_signal_emit(&wd->pointer->pointer->events.button, &ev);
        wl_signal_emit(&wd->pointer->pointer->events.frame, NULL);
        return false;
    });

    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void buttondown(struct wl_client *client, struct wl_resource *resource, const char *button)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    int buttoncode = libevdev_event_code_from_name(EV_KEY, (std::string("BTN_") + button).c_str());
    wlr_event_pointer_button ev;

    if (buttoncode == -1)
    {
        return;
    }

    ev.device    = wd->pointer;
    ev.button    = buttoncode;
    ev.state     = WLR_BUTTON_PRESSED;
    ev.time_msec = wf::get_current_time();
    wl_signal_emit(&wd->pointer->pointer->events.button, &ev);
    wl_signal_emit(&wd->pointer->pointer->events.frame, NULL);

    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static void buttonup(struct wl_client *client, struct wl_resource *resource, const char *button)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    int buttoncode = libevdev_event_code_from_name(EV_KEY, (std::string("BTN_") + button).c_str());
    wlr_event_pointer_button ev;

    if (buttoncode == -1)
    {
        return;
    }

    ev.device    = wd->pointer;
    ev.button    = buttoncode;
    ev.state     = WLR_BUTTON_RELEASED;
    ev.time_msec = wf::get_current_time();
    wl_signal_emit(&wd->pointer->pointer->events.button, &ev);
    wl_signal_emit(&wd->pointer->pointer->events.frame, NULL);

    for (auto r : wd->client_resources)
    {
        wf_ctrl_base_send_ack(r);
    }
}

static const struct wf_ctrl_base_interface wayfire_control_impl =
{
    .maximize                = maximize,
    .unmaximize              = unmaximize,
    .minimize                = minimize,
    .unminimize              = unminimize,
    .focus                   = focus,
    .close                   = close,
    .move                    = move,
    .resize                  = resize,
    .ws_switch_view_append   = ws_switch_view_append,
    .ws_switch               = ws_switch,
    .ws_switch_abs           = ws_switch_abs,
    .keystroke               = keystroke,
    .keydown                 = keydown,
    .keyup                   = keyup,
    .buttonstroke            = buttonstroke,
    .buttondown              = buttondown,
    .buttonup                = buttonup
};

static void destroy_client(wl_resource *resource)
{
    wayfire_control *wd = (wayfire_control*)wl_resource_get_user_data(resource);

    for (auto& r : wd->client_resources)
    {
        if (r == resource)
        {
            r = nullptr;
        }
    }
    wd->client_resources.erase(std::remove(wd->client_resources.begin(),
        wd->client_resources.end(), nullptr), wd->client_resources.end());
}

static void bind_manager(wl_client *client, void *data,
    uint32_t version, uint32_t id)
{
    wayfire_control *wd = (wayfire_control*)data;

    auto resource =
        wl_resource_create(client, &wf_ctrl_base_interface, 1, id);
    wl_resource_set_implementation(resource,
        &wayfire_control_impl, data, destroy_client);
    wd->client_resources.push_back(resource);
    
}
