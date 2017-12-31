
#ifndef DRIVER_HH_
# define DRIVER_HH_

# include <string>
# include <iostream>
# include <fstream>

namespace parse
{
    /// Forward declarations of classes
    class Parser;
    class Scanner;
    class location;

    class Driver
    {
        public:
            Driver();
            ~Driver();

            int parse();
            int parse_file(const std::string& path);
            int parse_string(const std::string& value);

            void reset();

        private:
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

