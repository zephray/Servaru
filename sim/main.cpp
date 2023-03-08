//
// Servaru simulator
// Copyright 2023 Wenting Zhang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h> 

#include "verilated.h"
#include "verilated_vcd_c.h"
#include "Vsimtop.h"
#include "Vsimtop___024root.h"

#define CLK_PERIOD_PS 10000

// Verilator related
static Vsimtop *core;
static VerilatedVcdC *trace;
static uint64_t tickcount;

// Settings
static bool enable_trace = false;
static bool unlimited = true;
static bool verbose = false;
static uint64_t max_cycles;

static volatile sig_atomic_t running = 1;

#define CONCAT(a,b) a##b
#define SIGNAL(x) CONCAT(core->rootp->simtop__DOT__asictop__DOT__risu__DOT__cpu__DOT__,x)

double sc_time_stamp() {
    // This is in pS. Currently we use a 10ns (100MHz) clock signal.
    return (double)tickcount * (double)CLK_PERIOD_PS;
}

static void sig_handler(int _) {
    (void)_;
    running = 0;
}

void tick() {
    // Software simulated parts should read the signal
    // before clock edge (simulate the combinational
    // path), but only put the data out after the
    // clock edge (DFF)

    core->clk = 1;
    core->eval();

    // Let combinational changes propagate
    core->eval();
    if (enable_trace)
        trace->dump(tickcount * CLK_PERIOD_PS);

    core->clk = 0;
    
    core->eval();
    if (enable_trace)
        trace->dump(tickcount * CLK_PERIOD_PS + CLK_PERIOD_PS / 2);

    tickcount++;
}

void reset() {
    core->rst = 0;
    tick();
    core->rst = 1;
    tick();
    core->rst = 0;
}

int main(int argc, char *argv[]) {
    // Initialize testbench
    Verilated::commandArgs(argc, argv);
    core = new Vsimtop;
    Verilated::traceEverOn(true);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printf("Servaru Simulator\n");
            printf("Available parameters:\n"); 
            printf("    --trace: Enable waveform trace\n"); 
            printf("    --cycles <maxcycles>: Set simulation cycle limit\n");
            printf("    --verbose: Enable verbose output\n");
            exit(0);
        }
        else if (strcmp(argv[i], "--trace") == 0) {
            enable_trace = true;
        }
        else if (strcmp(argv[i], "--cycles") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Error: no cycle limit number provided\n");
                exit(1);
            }
            else {
                unlimited = false;
                max_cycles = atoi(argv[i + 1]);
            }
        }
        else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }

    if (enable_trace) {
        trace = new VerilatedVcdC;
        core->trace(trace, 99);
        trace->open("trace.vcd");
    }

    // Start simulation
    if (verbose)
        printf("Simulation start.\n");

    clock_t time = clock();

    reset();

    signal(SIGINT, sig_handler);

    while (running) {
        tick();
        
        if ((!unlimited) && (tickcount > max_cycles)) {
            break;
        }
    }

    time = clock() - time;
    time /= (CLOCKS_PER_SEC / 1000);
    if (time == 0) time = 1;

    printf("\nSimulation stopped after %ld cycles,\n"
            "average simulation speed: %ld kHz.\n",
            tickcount, tickcount / time);

    if (enable_trace) {
        trace->close();
    }

    return 0;
}