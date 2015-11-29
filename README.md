fsm <a href="https://travis-ci.org/r-lyeh/fsm"><img src="https://api.travis-ci.org/r-lyeh/fsm.svg?branch=master" align="right" /></a>
===

FSM is a lightweight finite-state machine class. Both Hierarchical FSM and simple FSM implementations are provided.

### Features
- [x] Expressive. Basic usage around `on(state,trigger) -> do lambda` expression.
- [x] Tiny, cross-platform, stand-alone, header-only.
- [x] ZLIB/libPNG licensed.

### Links
[Finite-State Machines: Theory and Implementation](http://gamedevelopment.tutsplus.com/tutorials/finite-state-machines-theory-and-implementation--gamedev-11867)

### Showcase
```c++
// basic hfsm sample

#include <iostream>
#include "fsm.hpp"

// custom states (gerunds) and actions (infinitives)

enum {
    walking = 'WALK',
    defending = 'DEFN',

    tick = 'tick',
};

struct ant_t {
    fsm::stack fsm;
    int health, distance, flow;

    ant_t() : health(0), distance(0), flow(1) {
        // define fsm transitions: on(state,trigger) -> do lambda
        fsm.on(walking, 'init') = [&]( const fsm::args &args ) {
            std::cout << "initializing" << std::endl;
        };
        fsm.on(walking, 'quit') = [&]( const fsm::args &args ) {
            std::cout << "exiting" << std::endl;
        };
        fsm.on(walking, 'push') = [&]( const fsm::args &args ) {
            std::cout << "pushing current task." << std::endl;
        };
        fsm.on(walking, 'back') = [&]( const fsm::args &args ) {
            std::cout << "back from another task. remaining distance: " << distance << std::endl;
        };
        fsm.on(walking, tick) = [&]( const fsm::args &args ) {
            std::cout << "\r" << "\\|/-"[ distance % 4 ] << " walking " << (flow > 0 ? "-->" : "<--") << " ";
            distance += flow;
            if( 1000 == distance ) {
                std::cout << "at food!" << std::endl;
                flow = -flow;
            }
            if( -1000 == distance ) {
                std::cout << "at home!" << std::endl;
                flow = -flow;
            }
        };
        fsm.on(defending, 'init') = [&]( const fsm::args &args ) {
            health = 1000;
            std::cout << "somebody is attacking me! he has " << health << " health points" << std::endl;
        };
        fsm.on(defending, tick) = [&]( const fsm::args &args ) {
            std::cout << "\r" << "\\|/-$"[ health % 4 ] << " health: (" << health << ")   ";
            --health;
            if( health < 0 ) {
                std::cout << std::endl;
                fsm.pop();
            }
        };

        // set initial fsm state
        fsm.set( walking );
    }
};

int main() {
    ant_t ant;
    for(int i = 0; i < 12000; ++i) {
        if( 0 == rand() % 10000 ) {
            ant.fsm.push(defending);
        }
        ant.fsm.command(tick);
    }
}
```

### Changelog
- v1.0.0 (2015/11/29): Code revisited to use fourcc integers (much faster); clean ups suggested by Chang Qian
- v0.0.0 (2014/02/15): Initial version
