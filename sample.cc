#include <iostream>
#include "fsm.hpp"

// available states and triggers

const fsm::state
    opened("opened"), closed("closed"),
    waiting("waiting"), playing("playing");

const fsm::trigger
    open("open"), close("close"),
    play("play"), stop("stop"),
    insert("insert"), extract("extract");

class cd_player : public fsm::core
{
    // implementation conditions / guards
    bool good_disk_format() { return true; }

    // implementation actions
    void open_tray()      { std::cout <<       "opening tray" << std::endl; }
    void close_tray()     { std::cout <<       "closing tray" << std::endl; }
    void get_cd_info()    { std::cout << "retrieving CD info" << std::endl; }
    void start_playback() { std::cout <<      "playing track" << std::endl; }

    // implementation variables
    bool got_cd;

    public:

    // setup

    cd_player() : got_cd(false)
    {}

    // transitions (logic)

    virtual fsm::state first() {
        return closed;
    }

    virtual fsm::state next() {
        return
        is(opened) && did(close) ? (close_tray(), got_cd ? get_cd_info(), waiting : closed) :
        is(opened) && did(insert) ? (got_cd = true, opened) :
        is(opened) && did(extract) ? (got_cd = false, opened) :

        is(closed) && did(open) ? (open_tray(), opened) :

        is(waiting) && did(play) && good_disk_format() ? (start_playback(), playing) :
        is(waiting) && did(open) ? (open_tray(), opened) :

        is(playing) && did(open) ? (open_tray(), opened) :
        is(playing) && did(stop) ? (closed) :
        fsm::state();
    }
};

int main( int argc, const char **argv ) {
    // create fsm
    //fsm cd;
    cd_player cd;

    // register states and triggers
    cd << opened << closed << waiting << playing;
    cd << play << open << close << stop << insert << extract;

    // create logic (@todo)
    // cd.first = []() {};
    // cd.next = []() {};

    // streaming configuration (optional)
    cd.on_warning = []( const std::string &line ) {
        std::cout << line << std::endl;
    };
    cd.on_verbose = []( const std::string &line ) {
        std::cout << line << std::endl;
    };

    for(;;) {
        char cmd;
        std::cout << "[" << cd.get_current_state() << "] ";
        std::cout << "(o)pen lid/(c)lose lid, (i)nsert cd/(e)xtract cd, (p)lay/(s)top cd, (q)uit? ";
        std::cin >> cmd;

        switch( cmd )
        {
            case 'p': play(cd); break;
            case 'o': open(cd); break;
            case 'c': close(cd); break;
            case 's': stop(cd); break;
            case 'i': insert(cd); break;
            case 'e': extract(cd); break;

            case 'q': return 0;
            default : std::cout << "what?" << std::endl;
        }
    }
}
