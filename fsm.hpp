/*
 * Simple FSM/HFSM class
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
 * - GOAP? behavior trees?
 * - trigger args
 * - counters

 * - rlyeh
 */

// - note on child states (tree of fsm's):
//   - triggers are handled to the most inner active state in the decision tree
//   - unhandled triggers are delegated to the parent state handler until handled or discarded by root state

#pragma once

#include <deque>
#include <functional>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace fsm
{
    template<typename T>
    inline std::string to_string( const T &t ) {
        std::stringstream ss;
        return ss << t ? ss.str() : std::string();
    }

    template<>
    inline std::string to_string( const std::string &t ) {
        return t;
    }

    struct args : public std::vector< std::string > {
        args() : std::vector< std::string >()
        {}
        template<typename T0>
        args( const T0 &t0 )  : std::vector< std::string >(1) {
            (*this)[0] = fsm::to_string(t0);
        }
        template<typename T0, typename T1>
        args( const T0 &t0, const T1 &t1 ) : std::vector< std::string >(2) {
            (*this)[0] = fsm::to_string(t0);
            (*this)[1] = fsm::to_string(t1);
        }
    };

    class state;
    typedef std::function< void( const fsm::args &args ) > code;

    class state : public std::string {
    public:

        fsm::args args;

        explicit
        state( const std::string &name = std::string() ) : std::string(name)
        {}

        state operator()() const {
            state self = *this;
            self.args = fsm::args();
            return self;
        }
        template<typename T0>
        state operator()( const T0 &t0 ) const {
            state self = *this;
            self.args = fsm::args(t0);
            return self;
        }
        template<typename T0, typename T1>
        state operator()( const T0 &t0, const T1 &t1 ) const {
            state self = *this;
            self.args = fsm::args(t0,t1);
            return self;
        }

        protected: std::vector< fsm::state > stack;
    };

    struct transition {
        fsm::state previous, trigger, current;
    };

    class stack {
    public:

        stack( const fsm::state &start = fsm::state("{undefined}") ) : deque(1) {
            deque[0] = start;
            begin( deque.back() );
        }

        ~stack() {
            while( size() ) {
                pop();
            }
        }

        void push( const fsm::state &state ) {
            if( deque.size() && deque.back() == state ) {
                return;
            }
            // queue
            pause( deque.back() );
            deque.push_back( state );
            begin( deque.back() );
        }
        void next( const fsm::state &state ) {
            if( deque.size() ) {
                follow( deque.back(), state );
            } else {
                push(state);
            }
        }
        void pop() {
            if( deque.size() ) {
                end( deque.back() );
                deque.pop_back();
            }
            if( deque.size() ) {
                resume( deque.back() );
            }
        }
        size_t size() const {
            return deque.size();
        }
        void start(const fsm::state &state) {
            next( state );
        }

        bool exec( const fsm::state &trigger ) {
            size_t size = this->size();
            if( !size ) {
                return false;
            }
            current_trigger = fsm::state();
            std::deque< rit > cancelled;
            for( rit it = deque.rbegin(); it != deque.rend(); ++it ) {
                fsm::state &self = *it;
                if( !call(self,trigger) ) {
                    cancelled.push_back(it);
                    continue;
                }
                for( std::deque< rit >::iterator it = cancelled.begin(), end = cancelled.end(); it != end; ++it ) {
                    (*it)->end();
                    deque.erase(--(it->base()));
                }
                current_trigger = trigger;
                return true;
            }
            return false;
        }

        // [] classic behaviour: "hello"[5] = undefined, "hello"[-1] = undefined
        // [] extended behaviour: "hello"[5] = h, "hello"[-1] = o, "hello"[-2] = l
        fsm::state get_state( signed int pos = -1 ) const {
            signed size = (signed)(deque.size());
            if( !size ) {
                return fsm::state();
            }
            return *( deque.begin() + ( pos >= 0 ? pos % size : size - 1 + ((pos+1) % size) ) );
        }
        fsm::transition get_log( signed int pos = -1 ) const {
            signed size = (signed)(log.size());
            if( !size ) {
                return fsm::transition();
            }
            return *( log.begin() + ( pos >= 0 ? pos % size : size - 1 + ((pos+1) % size) ) );
        }
        std::string get_trigger() const {
            return current_trigger;
        }

        bool is( const fsm::state &state ) const {
            return deque.empty() ? false : ( deque.back() == state );
        }

        /* (idle)___(trigger)__/''(hold)''''(release)''\__
        bool is_idle()      const { return transition.previous == transition.current; }
        bool is_triggered() const { return transition.previous == transition.current; }
        bool is_hold()      const { return transition.previous == transition.current; }
        bool is_released()  const { return transition.previous == transition.current; } */

        std::string debug() const {
            std::string out;
            for( cit it = deque.begin(), end = deque.end(); it != end; ++it ) {
                out += (*it) + ',';
            }
            return out;
        }

        bool call( const fsm::state &from, const fsm::state &to ) /*const*/ {
            std::map< bistate, fsm::code >::const_iterator code = callbacks.find(bistate(from,to));
            if( code == callbacks.end() ) {
                return false;
            } else {

                log.push_back( { from, current_trigger, to } );
                if( log.size() > 50 ) {
                    log.pop_front();
                }

                code->second( to.args );
                return true;
            }
        }

        fsm::code &on( const fsm::state &from, const fsm::state &to ) {
            return callbacks[ bistate(from,to) ] = callbacks[ bistate(from,to) ];
        }

        bool operator()( const fsm::state &trigger ) {
            return exec( trigger );
        }

    protected:

        void begin( const fsm::state &state ) {
            call( state, fsm::state("begin") );
        }

        void end( const fsm::state &state ) {
            call( state, fsm::state("end") );
        }

        void pause( const fsm::state &state ) {
            call( state, fsm::state("pause") );
        }

        void resume( const fsm::state &state ) {
            call( state, fsm::state("resume") );
        }

        void follow( fsm::state &current, const fsm::state &next ) {
            end( current );
            current = next;
            begin( current );
        }

        typedef std::pair<std::string, std::string> bistate;
        std::map< bistate, fsm::code > callbacks;

        std::deque< fsm::transition > log;
        std::deque< fsm::state > deque;
        fsm::state current_trigger;

        typedef std::deque< fsm::state >::const_iterator cit;
        typedef std::deque< fsm::state >::reverse_iterator rit;
    };
}
