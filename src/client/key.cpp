#include <cstdio>
#include <string>
#include <getopt.h>
#include <algorithm>
#include "wf-ctrl.hpp"

void do_key(WfCtrl *wd, int argc, char *argv[])
{
    std::string key;
    int delay = 12;

    struct option opts[] = {
        { "keystroke",   required_argument, NULL, 'k' },
        { "keydown",     required_argument, NULL, 'd' },
        { "keyup",       required_argument, NULL, 'u' },
        { "delay",       required_argument, NULL, 'm' },
        { 0,             0,                 NULL,  0  }
    };

    int c, i;
    while((c = getopt_long(argc - 1, argv + 1, "k:d:u:m:", opts, &i)) != -1)
    {
        switch(c)
        {
            case 'k':
                key = optarg;
                std::transform(key.begin(), key.end(), key.begin(), ::toupper);
                wf_ctrl_base_keystroke(wd->wf_control_manager, key.c_str(), delay);
                break;

            case 'd':
                key = optarg;
                std::transform(key.begin(), key.end(), key.begin(), ::toupper);
                wf_ctrl_base_keydown(wd->wf_control_manager, key.c_str());
                break;

            case 'u':
                key = optarg;
                std::transform(key.begin(), key.end(), key.begin(), ::toupper);
                wf_ctrl_base_keyup(wd->wf_control_manager, key.c_str());
                break;

            case 'm':
                delay = atoi(optarg);
                break;

            default:
                printf("Unsupported command line argument %s\n", optarg);
                return;
        }
    }

    wd->run();
}