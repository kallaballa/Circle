#include "NLHistory.h"
#include "NLCommand.h"
#include <cstddef>


NLHistory::NLHistory() : lastExecuted(-1), lastSaved(-1) {
}


NLHistory::~NLHistory() {
    for ( auto command : history ) {
        delete command;
    }
}


void NLHistory::clear() {
    for ( auto command : history ) {
        delete command;
    }
    history.clear();
    lastExecuted = -1;
    lastSaved = -1;
}


void NLHistory::save() {
    lastSaved = lastExecuted;
}


bool NLHistory::modified() {
    return lastSaved != lastExecuted;
}


void NLHistory::limit( unsigned int numCommands ) {
    while ( history.size() > numCommands ) {
        delete history[ 0 ];
        history.erase( history.begin() );
        if ( lastExecuted >= 0 ) {
            lastExecuted--;
        }
        if ( lastSaved >= 0 ) {
            lastSaved--;
        }
    }
}


void NLHistory::add( NLCommand* command, bool execute ) {
    // Remove all commands in the branch that is "cut off" by adding a new command after undo.
    if ( lastExecuted + 1 < history.size() ) {
        int count = (int)history.size() - ( lastExecuted + 1 );
        int begin = lastExecuted + 1;
        int end = begin + count;
        
        for ( size_t i=0; i < count; i++ ) {
            delete history[ begin + i ];
        }
        history.erase( history.begin() + begin, history.begin() + end );
        lastSaved = -1;
    }

    if ( execute ) {
        command->execute();
    }

    history.push_back( command );
    lastExecuted = (int)history.size() - 1;
}


void NLHistory::revert() {
    while ( lastExecuted > 0 ) {
        history[ lastExecuted ]->undo();
        lastExecuted--;
    }
}


void NLHistory::undo() {
    if ( lastExecuted >= 0 ) {
        if ( history.size() > 0 ) {
            history[ lastExecuted ]->undo();
            lastExecuted--;
        }
    }
}


void NLHistory::redo() {
    if ( lastExecuted + 1 < history.size() ) {
        history[ lastExecuted + 1 ]->execute();
        lastExecuted++;
    }
}


