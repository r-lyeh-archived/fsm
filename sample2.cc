// basic fsm, CD player sample

#include <iostream>
#include "fsm.hpp"

// common triggers (verbs). you must provide these for all FSMs
fsm::state begin("begin"), end("end"), pause("pause"), resume("resume");

// custom states (gerunds)
fsm::state
    opening("opening"), closing("closing"),
    waiting("waiting"), playing("playing");

// custom triggers (verbs)
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

        // define fsm transitions: on(state,trigger) -> do lambda
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

int main( int argc, const char **argv ) {

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
}
