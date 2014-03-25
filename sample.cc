#include <iostream>
#include "fsm.hpp"

using namespace fsm;

    //
    fsm::state begin("begin"), end("end"), pause("pause"), resume("resume");

    // available states and triggers
    fsm::state
        opening("opening"), closing("closing"),
        waiting("waiting"), playing("playing");

    fsm::state
        open("open"), close("close"),
        play("play"), stop("stop"),
        insert("insert"), eject("eject");

class cd_player
{
    // implementation conditions / guards
    bool good_disk_format() { return true; }

    // implementation actions
    void open_tray()      { std::cout <<       "opening tray" << std::endl; }
    void close_tray()     { std::cout <<       "closing tray" << std::endl; }
    void get_cd_info()    { std::cout << "retrieving CD info" << std::endl; }
    void start_playback( const std::string &track ) { std::cout << "playing track #" << track << std::endl; }

    // implementation variables
    bool has_cd;

    public:

    fsm::stack fsm;

    cd_player() : has_cd(false)
    {
        // set initial fsm state
        fsm = opening;

        // define states
        fsm.on(opening,close) = [&]( const fsm::args &args ) {
            close_tray();
            if( !has_cd ) {
                fsm.start( closing );
            } else {
                get_cd_info();
                fsm.start( waiting );
            }
        };
        fsm.on(opening,insert) = [&]( const fsm::args &args ) {
            has_cd = true;
            fsm.start( opening );
        };
        fsm.on(opening,eject) = [&]( const fsm::args &args ) {
            has_cd = false;
            fsm.start( opening );
        };

        fsm.on(closing,open) = [&]( const fsm::args &args ) {
            open_tray();
            fsm.start( opening );
        };

        fsm.on(waiting,play) = [&]( const fsm::args &args ) {
            if( !good_disk_format() ) {
                fsm.start( waiting );
            } else {
                start_playback( args[0] );
                fsm.start( playing );
            }
        };
        fsm.on(waiting,open) = [&]( const fsm::args &args ) {
            open_tray();
            fsm.start( opening );
        };

        fsm.on(playing,open) = [&]( const fsm::args &args ) {
            open_tray();
            fsm.start( opening );
        };
        fsm.on(playing,stop) = [&]( const fsm::args &args ) {
            fsm.start( waiting );
        };

    }
};

fsm::state update("udpate");
fsm::state walking("walking"), attacking("attacking");

class ant {
public:
        fsm::stack fsm;
        int health, distance, flow;

        ant() : health(0), distance(0), flow(1) {
            // initial state
            fsm = walking;

            // setup
            fsm.on(walking,resume) = [&]( const fsm::args &args ) {
                std::cout << "pre-walk! distance:" << distance << std::endl;
            };
            fsm.on(walking,update) = [&]( const fsm::args &args ) {
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
            fsm.on(attacking,begin) = [&]( const fsm::args &args ) {
                std::cout << "pre-attack!" << std::endl;
                health = 1000;
            };
            fsm.on(attacking,update) = [&]( const fsm::args &args ) {
                std::cout << "\r" << "\\|/-$"[ distance % 4 ] << " fighting !! hp:(" << health << ")   ";
                --health;
                if( health < 0 ) {
                    std::cout << std::endl;
                    fsm.pop();
                }
            };
        }
};

int main( int argc, const char **argv ) {

    // basic fsm sample
    cd_player cd;

    for(;;) {
        char cmd;
        std::cout << "[" << cd.fsm.get_state() << "] ";
        std::cout << "(o)pen lid/(c)lose lid, (i)nsert cd/(e)ject cd, (p)lay/(s)top cd, (q)uit? ";
        std::cin >> cmd;

        switch( cmd )
        {
            case 'p': cd.fsm(play(1+rand()%10)); break;
            case 'o': cd.fsm(open()); break;
            case 'c': cd.fsm(close()); break;
            case 's': cd.fsm(stop()); break;
            case 'i': cd.fsm(insert()); break;
            case 'e': cd.fsm(eject()); break;

            case 'q': break;
            default : std::cout << "what?" << std::endl;
        }
        if( cmd == 'q' ) break;
    }

    // hfsm ant sample
    ant mini;
    for(;;) {
        if( 0 == rand() % 10000 ) mini.fsm.push(attacking);
        mini.fsm(update());
    }
}
