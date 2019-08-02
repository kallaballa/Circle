#include <iostream>

#include "NLCommand.h"
#include "NLCommandGroup.h"
#include "NLHistory.h"
#include <assert.h>

class TestDocument {
public:
    std::string text;
    
    void print() const {
        std::cout << "Document text: " << text << "\n";
    }
};


class TestCommandBase : public NLCommand {
protected:
    TestDocument & document;
    
public:
    TestCommandBase( TestDocument & document ) : document( document ) {
    }
};


class TestCommandAppend : public TestCommandBase {
private:
    std::string s;
    
public:
    TestCommandAppend( TestDocument & document, std::string const & s ) : TestCommandBase(document), s(s) {
    }
    
    void execute() {
        document.text += s;
    }
    
    void undo() {
        auto len = document.text.length() - s.length();
        document.text.erase( len );
    }
};


class TestCommandBackspace : public TestCommandBase {
private:
    std::string deleted;
    
public:
    TestCommandBackspace( TestDocument & document ) : TestCommandBase(document) {
        deleted = document.text[ document.text.length() - 1 ];
    }
    
    void execute() {
        document.text.erase( document.text.length() - 1 );
    }
    
    void undo() {
        document.text += deleted;
    }
};



int main( int argc, char *argv[] ) {
    TestDocument document;
    document.text = "Hello, world";
    
    NLHistory history;
    
    document.print();
    
    assert( document.text == "Hello, world" );
    
    history.add( new TestCommandAppend( document, "!" ), true ); // !

    document.print();

    assert( document.text == "Hello, world!" );

    history.add( new TestCommandAppend( document, "!" ), true ); // !!

    document.print();

    assert( document.text == "Hello, world!!" );

    history.add( new TestCommandAppend( document, "!" ), true ); // !!!
    
    document.print();

    assert( document.text == "Hello, world!!!" );

    history.undo(); // !!
    
    document.print();
    
    assert( document.text == "Hello, world!!" );

    history.redo(); // !!!
    
    document.print();
    
    assert( document.text == "Hello, world!!!" );

    history.undo(); // !!
    history.undo(); // !
    
    document.print();
    
    assert( document.text == "Hello, world!" );

    history.add( new TestCommandAppend( document, "X" ), true ); // !X
    
    document.print();

    assert( document.text == "Hello, world!X" );

    history.undo(); // !

    document.print();

    assert( document.text == "Hello, world!" );

    history.undo(); //

    document.print();

    assert( document.text == "Hello, world" );

    history.redo(); // !

    document.print();

    assert( document.text == "Hello, world!" );

    history.redo(); // !X

    document.print();
    
    assert( document.text == "Hello, world!X" );

    history.redo(); // !X
    history.redo(); // !X
    history.redo(); // !X

    assert( document.text == "Hello, world!X" );

    history.undo(); //
    history.undo(); //
    history.undo(); //
    history.undo(); //
    history.undo(); //
    history.undo(); //
    history.undo(); //
    history.undo(); //

    assert( document.text == "Hello, world" );
}

