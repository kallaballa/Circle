#pragma once



class NLCommand {
    
public:
    
	virtual void execute() = 0;
    
    
	virtual void undo() = 0;
    
    
    virtual ~NLCommand() { }

};
