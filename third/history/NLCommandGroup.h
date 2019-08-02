#pragma once


#include <vector>
#include "NLCommand.h"



class NLCommandGroup : public NLCommand {
    
private:
    
    std::vector<NLCommand*> commands;
    
    
public:

    ~NLCommandGroup();
    void add( NLCommand* command );
    void execute();
    void undo();
    
    
};
