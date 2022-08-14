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
}

wayfire_control::~wayfire_control()
{
    wl_global_destroy(manager);
}

wayfire_view view_from_id(uint32_t id)
{
    for (auto& view : wf::get_core().get_all_views())
    {
        if (view->get_id() == id)
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

static const struct wf_ctrl_base_interface wayfire_control_impl =
{
    .maximize                = maximize,
    .unmaximize              = unmaximize,
    .minimize                = minimize,
    .unminimize              = unminimize,
    .focus                   = focus,
    .move                    = move,
    .resize                  = resize,
    .ws_switch_view_append   = ws_switch_view_append,
    .ws_switch               = ws_switch,
    .ws_switch_abs           = ws_switch_abs
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
