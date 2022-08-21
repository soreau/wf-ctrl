#include <cstdio>
#include <getopt.h>
#include "wf-ctrl.hpp"

void do_mousemove(WfCtrl *wd, int argc, char *argv[])
{
    int x, y;

    struct option opts[] = {
        { "mousemove",      required_argument, NULL, 'm' },
        { 0,                0,                 NULL,  0  }
    };

    int c, i;
    while((c = getopt_long(argc - 1, argv + 1, "m:", opts, &i)) != -1)
    {
        switch(c)
        {
            case 'm':
                if (sscanf(optarg, "%d,%d", &x, &y) != 2)
                {
                    break;
                }
                wf_ctrl_base_mousemove(wd->wf_control_manager, x, y);
                break;

            default:
                printf("Unsupported command line argument %s\n", optarg);
                return;
        }
    }

    wd->run();
}