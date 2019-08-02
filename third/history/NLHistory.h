#pragma once


#include <vector>


class NLCommand;



class NLHistory {
    
private:
    
    std::vector<NLCommand*> history;
    int lastExecuted;
    int lastSaved;
    
public:
    
    NLHistory();
    ~NLHistory();
    
    void clear();
    void save();
    bool modified();
    void limit( unsigned int numCommands );
    void add( NLCommand* command, bool execute );
    void revert();
    void undo();
    void redo();
    
};
