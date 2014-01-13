/*
 * Simple FSM class
 * Copyright (c) 2011, 2012, 2013, 2014 Mario 'rlyeh' Rodriguez

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.

 * - inspiration:   BOOST MSM (eUML FSM)
 *   format:        source + event [guard(s)] [/ action(s)] == target
 *   original sample:
 *
 *   BOOST_MSM_EUML_TRANSITION_TABLE((
 *   Stopped + play [some_guard] / (some_action , start_playback)  == Playing ,
 *   Stopped + open_close/ open_drawer                             == Open    ,
 *   Stopped + stop                                                == Stopped ,
 *   Open    + open_close / close_drawer                           == Empty   ,
 *   Empty   + open_close / open_drawer                            == Open    ,
 *   Empty   + cd_detected [good_disk_format] / store_cd_info      == Stopped
 *   ),transition_table)
 *
 * - proposal:
 *   format:        return
 *                  current_state && trigger [&& guard(s)] ? ( [action(s),] new_state) :
 *                  [...]
 *                  0
 *   sample becomes:
 *
 *   return
 *   Stopped && play && some_guard()            ? (some_action(), start_playback(), Playing) :
 *   Stopped && open_close                      ? (open_drawer(), Open) :
 *   Stopped && stop                            ? (Stopped) :
 *   Open && open_close                         ? (close_drawer(), Empty) :
 *   Empty && open_close                        ? (open_drawer(), Open) :
 *   Empty && cd_detected && good_disk_format() ? (store_cd_info(), Stopped) :
 *   0

 * refs:
 * - http://en.wikipedia.org/wiki/Finite-state_machine

 * todo:
 * - HFSM? GOAP? behavior trees?
 * - stack: HFSM
 * - substates: state {0,1,2...}
 * - counters

 * - rlyeh
 */

#pragma once

#include <deque>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace fsm
{
    // trigger and state are both child classes, which are named strings

    template< unsigned id >
    struct child : public std::string {
        explicit
        child( const std::string &name = std::string() ) : std::string(name)
        {}

        template<typename T>
        bool operator()( T &t ) const {
            return id == 1 ? t( *this ) : false;
        }
    };

    typedef child<0> state;
    typedef child<1> trigger;

    typedef std::deque< state > states_history;
    typedef std::deque< trigger > triggers_history;

class core
{
public:

    // methods

    core()
    {}

    virtual ~core()
    {}

    core &add_trigger( const fsm::trigger &name ) {
        return preconfig(), *this;
    }

    core &add_state( const fsm::state &name ) {
        return preconfig(), *this;
    }

    state get_state( int pos = 0 ) const {
        signed size = signed(states.size());
        if( !size ) {
            static fsm::state invalid;
            return invalid = fsm::state("");
        }
        return states[ pos >= 0 ? pos % size : size - 1 + ((pos+1) % size) ];
    }

    trigger get_trigger( int pos = 0 ) const {
        signed size = signed(triggers.size());
        if( !size ) {
            static fsm::trigger invalid;
            return invalid = fsm::trigger("");
        }
        return triggers[ pos >= 0 ? pos % size : size - 1 + ((pos+1) % size) ];
    }

    const   states_history &get_states_history()   const { return states;   }
    const triggers_history &get_triggers_history() const { return triggers; }

    bool has_triggered() const { return !triggers.back().empty(); }
    void clear_trigger_flag() { triggers.push_back( fsm::trigger("") ); }

    // transitions to override

    virtual state first() = 0;
    virtual state next() = 0;

    // optional callbacks to override

    std::function< fsm::state( void ) > on_first;
    std::function< fsm::state( void ) > on_next;
    std::function< void( std::string ) > on_warning;
    std::function< void( std::string ) > on_verbose;

    // sugars:

    core &operator<<( const state &state     ) { return add_state( state );     }
    core &operator<<( const trigger &trigger ) { return add_trigger( trigger ); }

      state get_current_state()     const { return states.front(); }
      state get_previous_state()    const { return *++states.begin(); }
    trigger get_current_trigger()   const { return triggers.front(); }
    trigger get_previous_trigger()  const { return *++triggers.begin(); }

    // fsm engine core

    bool did( const fsm::trigger &trigger ) const {
        return trigger == triggers.front();
    }

    bool is( const fsm::state &state ) const {
        return state == states.front();
    }

    bool operator[]( const fsm::state &state ) const {
        return is( state );
    }

    bool exec( const fsm::trigger &trigger ) {
        return operator()( trigger );
    }

    bool operator()( const fsm::trigger &trigger ) {

        triggers.push_front( trigger );
        state current = next();
        triggers.pop_front();

        if( current.empty() ) {
            if( on_warning ) {
                std::stringstream line;
                line << "<fsm/fsm.hpp> says: [FAIL] invalid fsm::trigger " << trigger << "() raised on state [" << get_current_state() << "]";
                on_warning( line.str() );
            }
            return false;
        }

        // info
        std::stringstream line;
        line << "[" << get_current_state() << "]->" << trigger << "->[" << current << "]";

        log.push_back( line.str() );

        if( log.size() > 60 )
            log.pop_front();

        if( on_verbose ) {
            on_verbose( std::string() + "<fsm/fsm.hpp> says: " + log.back() );
        }

        // triggers history generation; push no dupes; max fixed size of 60
        if( triggers.empty() || triggers.front() != trigger )
            triggers.push_front( trigger );

        if( triggers.size() > 60 )
            triggers.pop_back();

        // states history generation; push no dupes; max fixed size of 60
        if( states.empty() || states.front() != current )
            states.push_front( current );

        if( states.size() > 60 )
            states.pop_back();

        return true;
    }

    // debug

    std::deque< std::string > get_log() const {
        return log;
    }

    // fsm details

    protected:

        states_history   states;
        triggers_history triggers;
        std::deque< std::string > log;

        void preconfig() {
            // init history
            states = states_history({ first(), fsm::state() });
            triggers = triggers_history({ fsm::trigger() });
        }
}; // fsm::core
}  // ::fsm
