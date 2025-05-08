#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include "process.h"

// Input Parser class
class Parser {
private:
    std::istream& input;
    
public:
    Parser(std::istream& in);
    
    // Parse input file and return processes and context switch time
    bool parse(std::vector<std::shared_ptr<Process>>& processes, int& contextSwitchTime);
    
    // Parse command line arguments
    static bool parseCommandLine(int argc, char* argv[], 
                                bool& detailedMode, bool& verboseMode, std::string& algorithm);
};

#endif // PARSER_H