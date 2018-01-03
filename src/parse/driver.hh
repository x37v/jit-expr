#ifndef DRIVER_HH_
# define DRIVER_HH_

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

            void reset();

        protected:
            void add_tree(xnor::ast::NodePtr root);

        private:
            TreeVector mTrees;
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

