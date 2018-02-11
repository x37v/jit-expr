//Copyright (c) 2017 Rémi Berson, modified for jit-expr 2018 by Alex Norman

#ifndef DRIVER_HH_
#define DRIVER_HH_

#include <string>
#include <iostream>
#include <fstream>
#include "ast.h"

namespace parse
{
    /// Forward declarations of classes
    class Parser;
    class Scanner;
    class location;

    typedef std::vector<xnor::ast::NodePtr> TreeVector;
    class Driver
    {
        public:
            Driver();
            ~Driver();

            TreeVector parse();
            TreeVector parse_file(const std::string& path);
            TreeVector parse_string(const std::string& value);

            TreeVector trees() const;
            xnor::ast::VariableVector inputs() const;

            void reset();

        protected:
            void add_tree(xnor::ast::NodePtr root);
            xnor::ast::VariablePtr add_input(xnor::ast::VariablePtr var);
            void validate();

        private:
            TreeVector mTrees;
            xnor::ast::VariableVector mInputs;

            Scanner*      scanner_;
            Parser*       parser_;
            location*     location_;
            int           error_;


            /// Allows Parser and Scanner to access private attributes
            /// of the Driver class
            friend class  Parser;
            friend class  Scanner;
    };
}

#endif /* !DRIVER_HH_ */

