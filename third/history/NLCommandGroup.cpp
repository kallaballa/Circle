#include "NLCommandGroup.h"


NLCommandGroup::~NLCommandGroup() {
    for ( auto command : commands ) {
        delete command;
    }
}

void NLCommandGroup::add( NLCommand* command ) {
    commands.push_back( command );
}


void NLCommandGroup::execute() {
    for ( unsigned int i=0; i < commands.size(); i++ ) {
        commands[ i ]->execute();
    }
}


void NLCommandGroup::undo() {
    for ( unsigned int i=0; i < commands.size(); i++ ) {
        commands[ i ]->undo();
    }
}

