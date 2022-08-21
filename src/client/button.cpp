#include <cstdio>
#include <getopt.h>
#include <algorithm>
#include "wf-ctrl.hpp"

void do_button(WfCtrl *wd, int argc, char *argv[])
{
    std::string key;
    int delay = 12;

    struct option opts[] = {
        { "buttonstroke", required_argument, NULL, 'b' },
        { "buttondown",   required_argument, NULL, 'd' },
        { "buttonup",     required_argument, NULL, 'u' },
        { "delay",        required_argument, NULL, 'm' },
        { 0,              0,                 NULL,  0  }
    };

    int c, i;
    while((c = getopt_long(argc - 1, argv + 1, "b:d:u:m:", opts, &i)) != -1)
    {
        switch(c)
        {
            case 'b':
                key = optarg;
                std::transform(key.begin(), key.end(), key.begin(), ::toupper);
                wf_ctrl_base_buttonstroke(wd->wf_control_manager, key.c_str(), delay);
                break;

            case 'd':
                key = optarg;
                std::transform(key.begin(), key.end(), key.begin(), ::toupper);
                wf_ctrl_base_buttondown(wd->wf_control_manager, key.c_str());
                break;

            case 'u':
                key = optarg;
                std::transform(key.begin(), key.end(), key.begin(), ::toupper);
                wf_ctrl_base_buttonup(wd->wf_control_manager, key.c_str());
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